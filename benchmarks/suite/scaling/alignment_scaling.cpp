// PoolPro Scaling Benchmark Suite — Alignment Scaling
// Measures Pool allocate() performance against stdPool, the standard
// library's own pool allocator, across increasing alignment
// requirements.
//
// A 4096-byte alignment inflates the per-block footprint enough that
// letting either side accumulate unreleased allocations across a full
// iteration tier would exhaust real memory — stdPool included, since
// its default upstream is unbounded but not free: every aligned chunk
// it pulls stays resident until released. So both sides wrap around
// at the same cadence: Pool via reset() (O(1)) whenever it runs out,
// stdPool via periodic release() every kBlockCount calls. This keeps
// the steady-state allocation path under measurement at a bounded
// memory footprint for both.
//
// Covers:
// - allocate() at a 4-byte alignment
// - allocate() at a 64-byte alignment
// - allocate() at a 4096-byte alignment

#include <support/framework.h>

using namespace PoolPro;

namespace {
constexpr std::size_t kBlockSize = sizeof(void*);
constexpr std::size_t kBlockCount = 10'000;
constexpr std::size_t kAlignSmall = 4;
constexpr std::size_t kAlignMedium = 64;
constexpr std::size_t kAlignLarge = 4096;

// Allocates from `pool`, wrapping around via reset() if it's exhausted.
void* allocateWrapping(Pool<false>& pool) {
    void* p = pool.allocate();
    if (!p) [[unlikely]] {
        pool.reset();
        p = pool.allocate();
    }
    return p;
}

// Allocates from `pool`, releasing everything back to its upstream
// every `kBlockCount` calls so it never outgrows Pool's own footprint.
void* allocateStdWrapping(stdPool& pool, std::size_t size, std::size_t alignment,
                          std::size_t& counter) {
    if (counter == kBlockCount) [[unlikely]] {
        pool.release();
        counter = 0;
    }
    ++counter;
    return pool.allocate(size, alignment);
}
} // namespace

// Measures allocate() at a 4-byte alignment.
static void bench_alignment_small() {
    Pool<false> cSrc(kBlockSize, kBlockCount, kAlignSmall);
    stdPool sSrc;
    std::size_t sCounter = 0;

    auto c = [&] {
        void* p = allocateWrapping(cSrc);
        doNotOptimize(p);
    };

    auto s = [&] {
        void* p = allocateStdWrapping(sSrc, kBlockSize, kAlignSmall, sCounter);
        doNotOptimize(p);
    };

    BENCH("allocate() 4-byte alignment", c, s);
}

// Measures allocate() at a 64-byte alignment.
static void bench_alignment_medium() {
    Pool<false> cSrc(kBlockSize, kBlockCount, kAlignMedium);
    stdPool sSrc;
    std::size_t sCounter = 0;

    auto c = [&] {
        void* p = allocateWrapping(cSrc);
        doNotOptimize(p);
    };

    auto s = [&] {
        void* p = allocateStdWrapping(sSrc, kBlockSize, kAlignMedium, sCounter);
        doNotOptimize(p);
    };

    BENCH("allocate() 64-byte alignment", c, s);
}

// Measures allocate() at a 4096-byte alignment.
static void bench_alignment_large() {
    Pool<false> cSrc(kBlockSize, kBlockCount, kAlignLarge);
    stdPool sSrc;
    std::size_t sCounter = 0;

    auto c = [&] {
        void* p = allocateWrapping(cSrc);
        doNotOptimize(p);
    };

    auto s = [&] {
        void* p = allocateStdWrapping(sSrc, kBlockSize, kAlignLarge, sCounter);
        doNotOptimize(p);
    };

    BENCH("allocate() 4096-byte alignment", c, s);
}

static void run_benchmarks() {
    bench_alignment_small();
    std::cout << "\n";

    bench_alignment_medium();
    std::cout << "\n";

    bench_alignment_large();
}

REGISTER_BENCH_SUITE();
