// ============================================================
// fuzz/fuzz_pool.cpp
//
// Differential fuzzer for PoolPro::Pool, checked against a shadow
// model after every single operation (not just at the end) so a
// fuzzer-found failure localizes to the exact operation that caused it.
//
// The model is deliberately exact rather than approximate: it tracks
// the free list as a LIFO stack of block indices plus the watermark,
// so every pointer the pool hands back is compared against the one
// the model says must come next -- not merely "some valid block".
// That is what pins down the documented behavior ("free list is
// drained before the watermark is touched", "LIFO reuse", "virgin
// blocks are handed out in address order") instead of only checking
// that nothing crashed.
//
// Every live block is also filled with a per-block byte pattern (or,
// for create()d blocks, a Probe object plus a patterned tail) and
// re-verified after every operation. The whole pool is one big
// allocation, so ASan cannot see one block overlapping another --
// this pattern check is what catches a free-list node being written
// into a block that is still allocated, or a block handed out twice.
//
// Specifically targets:
//   - allocate()/deallocate() and allocateBatch()/deallocateBatch(),
//     including batches larger than the pool (exhaustion mid-batch),
//     zero-length batches, and batches that mix live blocks with
//     pointers the pool must silently skip
//   - the "not owned" contract: nullptr, stack addresses, interior
//     pointers, one-past-the-end / just-before-the-start, and blocks
//     belonging to a *different* pool must all be ignored by
//     deallocate()/deallocateBatch()/destroy() and rejected by owns()
//   - create()'s exception path: Probe's constructor throws on
//     request, and the harness checks the block was handed back
//     (net effect: on the free list, with alloc + dealloc both counted)
//     and that no Probe was leaked or double-destroyed
//   - destroy(): runs the destructor exactly once, then frees the block
//   - reset(), including stats being zeroed and every block becoming
//     virgin again
//   - move construction, move assignment (into a live pool, into a
//     moved-from pool, and self-move-assignment), and that a
//     moved-from pool really is a valid zero-capacity pool
//   - both Pool<false> and Pool<true>; for the latter, every Stats
//     counter is compared against the model after every operation
//   - configurations chosen so both owns() paths run (power-of-two
//     stride -> mask, other strides -> modulo) and so stride rounding
//     (blockSize not a multiple of alignment) is exercised
//
// Deliberately NOT covered yet: alignments below alignof(void*), and
// create() on a moved-from pool. Both are contract edges rather than
// bugs this harness can meaningfully differential-test -- see
// FUZZING.md, "Known sharp edges".
// ============================================================

// Keep AP_PRE/AP_ASSERT (which map to assert()) live regardless of what
// flags the fuzz build passes, so a contract violation aborts instead of
// silently turning into undefined behavior.
#ifdef NDEBUG
#undef NDEBUG
#endif

#include <PoolPro/Pool.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <memory>
#include <new>
#include <span>
#include <utility>
#include <vector>

using PoolPro::Pool;

// Aborts (rather than throwing/returning) on mismatch so libFuzzer
// captures a minimal, precise reproducer, and prints which invariant
// broke and where so a crash log is readable without a debugger.
#define FUZZ_CHECK(cond)                                                                           \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            std::fprintf(stderr, "FUZZ_CHECK failed: %s (%s:%d)\n", #cond, __FILE__, __LINE__);    \
            std::abort();                                                                          \
        }                                                                                          \
    } while (0)

namespace {

constexpr std::size_t kNone = static_cast<std::size_t>(-1);

// An address that is never inside any pool's arena, for the "not owned" checks.
std::uint64_t gForeignWord = 0;

// ------------------------------------------------------------
// Probe: the type handed to create<T>()/destroy<T>().
//
// - Its constructor is not noexcept and can be told to throw, which
//   forces create() down its try/catch path.
// - `alive` counts constructed-and-not-yet-destroyed objects across
//   every pool, so a leaked or double-destroyed object shows up as a
//   count mismatch.
// - `alive` is only incremented after the throw check, matching real
//   C++ semantics: a constructor that throws never has its destructor
//   run, so it must not be counted.
// ------------------------------------------------------------
struct ProbeError {};

struct Probe {
    static inline std::size_t alive = 0;

    std::uint64_t magic;

    Probe(std::uint64_t m, bool fail) : magic(m) {
        if (fail)
            throw ProbeError{};
        ++alive;
    }
    ~Probe() { --alive; }

    Probe(const Probe&) = delete;
    Probe& operator=(const Probe&) = delete;
};
static_assert(sizeof(Probe) <= 8, "Probe must fit in the smallest block the harness builds");
static_assert(alignof(Probe) <= 8, "Probe must satisfy the smallest alignment the harness builds");

// ------------------------------------------------------------
// Input decoding.
// ------------------------------------------------------------
class Reader {
  public:
    Reader(const std::uint8_t* data, std::size_t size) : data_(data), size_(size) {}

    [[nodiscard]] bool empty() const { return size_ == 0; }

    // Returns 0 once the input is exhausted, so an op that needs a
    // parameter byte never has to special-case running out.
    std::uint8_t next() {
        if (size_ == 0)
            return 0;
        --size_;
        return *data_++;
    }

  private:
    const std::uint8_t* data_;
    std::size_t size_;
};

struct Config {
    std::size_t blockSize;
    std::size_t blockCount;
    std::size_t alignment;

    // Small tables so most runs hit exhaustion quickly, and so the fuzzer
    // can reach every combination with a few bytes.
    //  - sizes 9 and 33 are not multiples of any alignment, so stride
    //    rounding (alignForward) is exercised.
    //  - stride comes out a power of two for some combinations (owns()
    //    uses a mask) and not for others, e.g. 24/48 (owns() uses `%`).
    //  - alignments start at alignof(void*): the pool writes a FreeNode
    //    into each freed block, so smaller alignments are a contract
    //    edge, not something to differential-test (see FUZZING.md).
    static Config read(Reader& in) {
        static constexpr std::size_t kSizes[] = {8, 9, 16, 24, 33, 48, 64};
        static constexpr std::size_t kCounts[] = {1, 2, 3, 5, 16, 33};
        static constexpr std::size_t kAligns[] = {8, 16, 32, 64};

        Config c{};
        c.blockSize = kSizes[in.next() % std::size(kSizes)];
        c.blockCount = kCounts[in.next() % std::size(kCounts)];
        c.alignment = kAligns[in.next() % std::size(kAligns)];
        return c;
    }
};

// ------------------------------------------------------------
// Shadow model of one Pool.
//
// Blocks are tracked by *index*, not address. That keeps the model
// independent of where the arena landed, and it means a block can be
// tracked before its address is known (see locate()).
// ------------------------------------------------------------
struct Live {
    std::size_t index;
    std::uint8_t tag;     // every byte of the block (past the Probe, if any) holds this
    bool isObject;        // true if a Probe lives at the start of the block
    std::uint64_t magic;  // expected Probe::magic when isObject
};

struct Counters {
    std::size_t totalAllocated = 0;
    std::size_t peakUsed = 0;
    std::size_t allocations = 0;
    std::size_t deallocations = 0;
};

// The default-constructed Shadow is exactly the state a moved-from Pool
// must be in: zero capacity, nothing free, nothing live.
struct Shadow {
    std::size_t blockSize = 0;
    std::size_t stride = 0;
    std::size_t blockCount = 0;
    std::size_t alignment = 0;

    std::byte* base = nullptr;          // arena start; learned from the first pointer observed
    std::size_t watermark = 0;          // blocks [watermark, blockCount) are virgin
    std::vector<std::size_t> freeStack; // free-list contents; back() is the head
    std::vector<Live> live;             // currently allocated blocks
    Counters stats;                     // compared only when EnableStats
    std::uint8_t nextTag = 1;
    std::uint64_t nextMagic = 1;
};

template <bool EnableStats> class Harness {
  public:
    explicit Harness(const Config& cfg) : pool_(cfg.blockSize, cfg.blockCount, cfg.alignment) {
        s_.blockSize = cfg.blockSize;
        s_.blockCount = cfg.blockCount;
        s_.alignment = cfg.alignment;
        s_.stride = (cfg.blockSize + cfg.alignment - 1) & ~(cfg.alignment - 1);
    }

    // Moves carry the model along with the pool, and leave the source's
    // model in the same zero-capacity state the real moved-from pool is in.
    Harness(Harness&& other) noexcept
        : pool_(std::move(other.pool_)), s_(std::exchange(other.s_, Shadow{})) {}

    // Safe under self-assignment: the pool guards it itself, and the
    // exchange below moves the model out and straight back in.
    Harness& operator=(Harness&& other) noexcept {
        pool_ = std::move(other.pool_);
        s_ = std::exchange(other.s_, Shadow{});
        return *this;
    }

    // ---- allocation ----------------------------------------------------

    void allocate() {
        const std::size_t idx = consume(pool_.allocate());
        if (idx == kNone)
            return;
        noteAlloc(1);
        adoptRaw(idx);
    }

    // Keeps allocating until the pool must be exhausted, then a few more
    // times, so the "nullptr when exhausted" path is always exercised.
    void fill() {
        for (std::size_t i = 0; i <= s_.blockCount; ++i)
            allocate();
    }

    void allocateBatch(std::size_t n) {
        std::vector<void*> out(n, nullptr);
        const std::size_t count = pool_.allocateBatch(std::span<void*>(out));
        FUZZ_CHECK(count <= n);

        // Walk every slot -- including the ones past `count`, which must
        // still be untouched -- against what the model says comes next.
        std::size_t taken = 0;
        for (std::size_t k = 0; k < n; ++k) {
            const std::size_t idx = consume(out[k]);
            if (idx == kNone)
                continue; // consume() already checked out[k] == nullptr
            ++taken;
            adoptRaw(idx);
        }
        FUZZ_CHECK(count == taken);
        if (taken)
            noteAlloc(taken);
    }

    // ---- deallocation ----------------------------------------------------

    void deallocateOne(std::size_t pick) {
        if (s_.live.empty())
            return;
        const std::size_t at = pick % s_.live.size();
        const Live l = s_.live[at];
        checkLive(l);
        if (l.isObject) // deallocate() never runs destructors; that is the caller's job
            std::destroy_at(objectAt(l));

        pool_.deallocate(ptr(l.index));

        dropLive(at);
        s_.freeStack.push_back(l.index);
        ++s_.stats.deallocations;
    }

    // Frees up to `want` distinct live blocks in one call. If `withJunk`
    // is set, a pointer the pool must skip is interleaved after every
    // real one, so the "skip and keep going" path is exercised mid-batch.
    void deallocateBatch(std::size_t start, std::size_t want, bool withJunk) {
        const std::size_t n = std::min(want, s_.live.size());
        if (n == 0)
            return;

        const std::vector<void*> junk = foreignPointers(nullptr);
        std::vector<bool> taken(s_.live.size(), false);
        std::vector<std::size_t> order; // block indices, in the order they are passed
        std::vector<void*> ptrs;

        for (std::size_t k = 0; k < n; ++k) {
            const std::size_t at = (start + k) % s_.live.size(); // distinct: n <= live.size()
            const Live& l = s_.live[at];
            checkLive(l);
            if (l.isObject)
                std::destroy_at(objectAt(l));
            taken[at] = true;
            order.push_back(l.index);
            ptrs.push_back(ptr(l.index));
            if (withJunk)
                ptrs.push_back(junk[k % junk.size()]);
        }

        pool_.deallocateBatch(std::span<void*>(ptrs));

        // The pool pushes onto the free list in argument order, so the
        // last real pointer ends up as the head.
        for (std::size_t idx : order)
            s_.freeStack.push_back(idx);
        s_.stats.deallocations += order.size();

        std::vector<Live> kept;
        for (std::size_t i = 0; i < s_.live.size(); ++i)
            if (!taken[i])
                kept.push_back(s_.live[i]);
        s_.live = std::move(kept);
    }

    // ---- object lifecycle --------------------------------------------------

    void create(std::uint8_t param) {
        // On a moved-from pool blockSize_ is 0, so create()'s
        // AP_PRE(sizeof(T) <= blockSize_) cannot hold. See FUZZING.md.
        if (s_.blockCount == 0)
            return;

        const bool fail = (param & 1) != 0;
        const std::uint64_t magic = (std::uint64_t{param} << 56) | s_.nextMagic++;

        Probe* obj = nullptr;
        bool threw = false;
        try {
            obj = pool_.template create<Probe>(magic, fail);
        } catch (const ProbeError&) {
            threw = true;
        }

        const std::size_t idx = takeNext();
        if (idx == kNone) {
            // Exhausted: create() returns nullptr before ever running the
            // constructor, so even a "fail" request must not throw.
            FUZZ_CHECK(obj == nullptr && !threw);
            return;
        }

        noteAlloc(1);
        if (fail) {
            // The constructor threw: create() must have handed the block
            // straight back, so it sits on top of the free list.
            FUZZ_CHECK(threw && obj == nullptr);
            s_.freeStack.push_back(idx);
            ++s_.stats.deallocations;
            return;
        }

        FUZZ_CHECK(!threw && obj != nullptr);
        locate(obj, idx);
        const std::uint8_t tag = s_.nextTag++;
        std::memset(ptr(idx) + sizeof(Probe), tag, s_.blockSize - sizeof(Probe));
        s_.live.push_back({idx, tag, true, magic});
    }

    void destroyOne(std::size_t pick) {
        std::vector<std::size_t> objects; // positions in s_.live
        for (std::size_t i = 0; i < s_.live.size(); ++i)
            if (s_.live[i].isObject)
                objects.push_back(i);
        if (objects.empty())
            return;

        const std::size_t at = objects[pick % objects.size()];
        const Live l = s_.live[at];
        checkLive(l);

        const std::size_t before = Probe::alive;
        pool_.destroy(objectAt(l));
        FUZZ_CHECK(Probe::alive == before - 1); // destructor ran, exactly once

        dropLive(at);
        s_.freeStack.push_back(l.index);
        ++s_.stats.deallocations;
    }

    // ---- pool management ---------------------------------------------------

    void reset() {
        destroyAllObjects(); // reset() never runs destructors either
        pool_.reset();
        s_.watermark = 0;
        s_.freeStack.clear();
        s_.live.clear();
        s_.stats = Counters{};
    }

    // Runs the destructor of every live Probe. The pool never does this
    // itself, so the harness must before anything that discards a block
    // (reset, arena release, end of run) or `Probe::alive` would drift.
    void destroyAllObjects() {
        for (Live& l : s_.live) {
            if (!l.isObject)
                continue;
            checkLive(l);
            std::destroy_at(objectAt(l));
            l.isObject = false; // now just an allocated block with caller-owned bytes
            std::memset(ptr(l.index), l.tag, s_.blockSize);
        }
    }

    // ---- ownership ---------------------------------------------------------

    // Everything here is a pointer this pool must NOT claim: it must be
    // ignored by deallocate()/deallocateBatch()/destroy() with no change
    // to any state or statistic, and rejected by owns().
    //
    // `other`, if given, contributes a block from a different pool --
    // still "not owned" from this pool's point of view.
    [[nodiscard]] std::vector<void*> foreignPointers(const Harness* other) const {
        std::vector<void*> v{nullptr, &gForeignWord};
        if (s_.base) {
            const auto b = reinterpret_cast<std::uintptr_t>(s_.base);
            const std::size_t bytes = s_.stride * s_.blockCount;
            v.push_back(reinterpret_cast<void*>(b + bytes));                // one past the end
            v.push_back(reinterpret_cast<void*>(b - 1));                    // just before the start
            v.push_back(reinterpret_cast<void*>(b + 1));                    // inside block 0
            v.push_back(reinterpret_cast<void*>(b + bytes - 1));            // last byte of the arena
            v.push_back(reinterpret_cast<void*>(b + s_.stride - 1));        // last byte of block 0
        }
        if (other && !other->s_.live.empty())
            v.push_back(other->ptr(other->s_.live.front().index));
        return v;
    }

    void rejectForeign(const Harness& other) {
        std::vector<void*> junk = foreignPointers(&other);

        for (void* p : junk) {
            FUZZ_CHECK(!pool_.owns(p));
            pool_.deallocate(p);
        }
        pool_.deallocateBatch(std::span<void*>(junk));

        // destroy<T>() must skip a non-owned pointer *before* running the
        // destructor. Only pass pointers that are validly aligned for
        // Probe: the pool must never dereference them, and a misaligned
        // typed pointer is not worth arguing with UBSan over.
        for (void* p : junk) {
            if (reinterpret_cast<std::uintptr_t>(p) % alignof(Probe) == 0)
                pool_.destroy(static_cast<Probe*>(p));
        }
        // No model changes: verify() will catch any state or stat drift.
    }

    // owns() must be exactly "a block start inside the arena".
    void probeOwns(std::size_t pick) const {
        if (!s_.base) // nothing observed yet, so no in-arena address is known
            return;
        for (std::size_t i = 0; i < s_.blockCount; ++i)
            FUZZ_CHECK(pool_.owns(ptr(i))); // true for allocated, free and virgin blocks alike

        const std::size_t block = pick % s_.blockCount;
        const std::size_t offset = 1 + (pick / s_.blockCount) % (s_.stride - 1);
        FUZZ_CHECK(!pool_.owns(ptr(block) + offset)); // inside a block, not its start
    }

    // ---- verification --------------------------------------------------------

    void verify() const {
        FUZZ_CHECK(pool_.totalBlocks() == s_.blockCount);
        FUZZ_CHECK(pool_.blockStride() == s_.stride);
        FUZZ_CHECK(pool_.capacity() == s_.stride * s_.blockCount);

        const std::size_t freeNow = s_.freeStack.size() + (s_.blockCount - s_.watermark);
        FUZZ_CHECK(pool_.freeBlocks() == freeNow);
        FUZZ_CHECK(pool_.usedBlocks() == s_.blockCount - freeNow);
        FUZZ_CHECK(pool_.usedBlocks() == s_.live.size());
        FUZZ_CHECK(pool_.usedBlocks() + pool_.freeBlocks() == pool_.totalBlocks());

        if constexpr (EnableStats) {
            const auto& st = pool_.getStats();
            FUZZ_CHECK(st.totalAllocated_ == s_.stats.totalAllocated);
            FUZZ_CHECK(st.allocations_ == s_.stats.allocations);
            FUZZ_CHECK(st.peakUsed_ == s_.stats.peakUsed);
            FUZZ_CHECK(st.deallocations_ == s_.stats.deallocations);
            FUZZ_CHECK(st.allocations_ == st.totalAllocated_); // documented to be the same number
        }

        for (const Live& l : s_.live) {
            FUZZ_CHECK(pool_.owns(ptr(l.index)));
            checkLive(l);
        }
    }

    [[nodiscard]] std::size_t liveObjectCount() const {
        return static_cast<std::size_t>(
            std::count_if(s_.live.begin(), s_.live.end(), [](const Live& l) { return l.isObject; }));
    }

    // The pool must be in the zero-capacity, hands-off state after a move.
    void verifyMovedFrom() const {
        FUZZ_CHECK(s_.blockCount == 0 && s_.live.empty());
        verify();
    }

  private:
    // ---- model plumbing -------------------------------------------------------

    // Pops what the pool must hand out next: the free-list head if there
    // is one, otherwise the next virgin block. kNone means "exhausted".
    std::size_t takeNext() {
        if (!s_.freeStack.empty()) {
            const std::size_t idx = s_.freeStack.back();
            s_.freeStack.pop_back();
            return idx;
        }
        if (s_.watermark < s_.blockCount)
            return s_.watermark++;
        return kNone;
    }

    // Compares a pointer the pool returned against what the model says was
    // due, and advances the model. Returns the block index, or kNone.
    std::size_t consume(void* got) {
        const std::size_t idx = takeNext();
        if (idx == kNone) {
            FUZZ_CHECK(got == nullptr);
            return kNone;
        }
        FUZZ_CHECK(got != nullptr);
        locate(got, idx);
        return idx;
    }

    // Ties an observed pointer to its expected block index. The first
    // pointer ever seen also tells us where the arena is; after that
    // every pointer must land exactly on base + idx * stride.
    void locate(void* got, std::size_t idx) {
        auto* p = static_cast<std::byte*>(got);
        FUZZ_CHECK(reinterpret_cast<std::uintptr_t>(p) % s_.alignment == 0);
        if (!s_.base)
            s_.base = p - idx * s_.stride;
        FUZZ_CHECK(p == s_.base + idx * s_.stride);
    }

    void noteAlloc(std::size_t count) {
        s_.stats.totalAllocated += count;
        s_.stats.allocations += count;
        const std::size_t freeNow = s_.freeStack.size() + (s_.blockCount - s_.watermark);
        s_.stats.peakUsed = std::max(s_.stats.peakUsed, s_.blockCount - freeNow);
    }

    void adoptRaw(std::size_t idx) {
        const std::uint8_t tag = s_.nextTag++;
        std::memset(ptr(idx), tag, s_.blockSize);
        s_.live.push_back({idx, tag, false, 0});
    }

    void dropLive(std::size_t at) {
        s_.live[at] = s_.live.back();
        s_.live.pop_back();
    }

    [[nodiscard]] std::byte* ptr(std::size_t idx) const {
        FUZZ_CHECK(s_.base != nullptr);
        return s_.base + idx * s_.stride;
    }

    [[nodiscard]] Probe* objectAt(const Live& l) const {
        return std::launder(reinterpret_cast<Probe*>(ptr(l.index)));
    }

    // A live block must still hold exactly what the caller put there.
    void checkLive(const Live& l) const {
        const std::byte* p = ptr(l.index);
        std::size_t from = 0;
        if (l.isObject) {
            FUZZ_CHECK(objectAt(l)->magic == l.magic);
            from = sizeof(Probe);
        }
        for (std::size_t i = from; i < s_.blockSize; ++i)
            FUZZ_CHECK(std::to_integer<std::uint8_t>(p[i]) == l.tag);
    }

    Pool<EnableStats> pool_;
    Shadow s_;
};

template <bool EnableStats> void run(Reader& in) {
    using H = Harness<EnableStats>;

    H a(Config::read(in));
    H b(Config::read(in));
    H* const slots[2] = {&a, &b};

    while (!in.empty()) {
        const std::uint8_t byte = in.next();
        H& self = *slots[(byte >> 7) & 1];
        H& other = *slots[((byte >> 7) & 1) ^ 1];

        switch ((byte & 0x7F) % 14) {
        case 0:
            self.allocate();
            break;
        case 1:
            self.deallocateOne(in.next());
            break;
        case 2:
            // 0..35: includes the empty batch, and goes past the largest
            // pool (33 blocks) so a batch can run out of blocks mid-way.
            self.allocateBatch(in.next() % 36);
            break;
        case 3: {
            const std::size_t start = in.next();
            const std::uint8_t flags = in.next();
            self.deallocateBatch(start, flags & 0x3F, (flags & 0x80) != 0);
            break;
        }
        case 4:
            self.create(in.next());
            break;
        case 5:
            self.destroyOne(in.next());
            break;
        case 6:
            self.reset();
            break;
        case 7:
            self.fill();
            break;
        case 8:
            self.rejectForeign(other);
            break;
        case 9:
            self.probeOwns(in.next());
            break;
        case 10: { // move-construct out of `self`, check both halves, move back in
            H tmp(std::move(self));
            self.verifyMovedFrom();
            tmp.verify();
            self = std::move(tmp);
            tmp.verifyMovedFrom();
            break;
        }
        case 11: // move-assign over a live pool: its arena is released
            self.destroyAllObjects();
            self = std::move(other);
            other.verifyMovedFrom();
            break;
        case 12: { // self-move-assignment must be a no-op, not a self-destructive one
            H& alias = self;
            self = std::move(alias);
            break;
        }
        case 13: // replace the pool with a freshly built one (move-assign over a live pool)
            self.destroyAllObjects();
            self = H(Config::read(in));
            break;
        default:
            break;
        }

        a.verify();
        b.verify();
        FUZZ_CHECK(Probe::alive == a.liveObjectCount() + b.liveObjectCount());
    }

    // Nothing may be left constructed once both pools are done with.
    a.destroyAllObjects();
    b.destroyAllObjects();
    FUZZ_CHECK(Probe::alive == 0);
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    // 1 byte selects Pool<false>/Pool<true>, then 3 config bytes per pool.
    constexpr std::size_t kHeader = 1 + 3 + 3;
    if (size < kHeader)
        return 0;

    Reader in(data, size);
    if (in.next() & 1)
        run<true>(in);
    else
        run<false>(in);
    return 0;
}
