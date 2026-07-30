// PoolPro Core Benchmark Suite — Destroy
// Measures Pool destroy<T>() performance against stdPool,
// the standard library's own pool allocator, with placement new.
//
// Each case pre-constructs a block count sized generously above the
// LARGE iteration tier before timing starts, so the timed lambda only
// ever destroys an already-constructed object.
//
// Covers:
// - destroy<T>() with a trivial destructor
// - destroy<T>() with a non-trivial destructor

#include <support/framework.h>

#include <vector>

using namespace PoolPro;

namespace {
constexpr std::size_t kBlockCount = 2'000'000;

struct TrivialDtor {
    // Padded to sizeof(void*) so it satisfies Pool's minimum block size —
    // a free block must be able to hold a FreeNode.
    void* a = nullptr;
};

struct NonTrivialDtor {
    void* a = nullptr;
    ~NonTrivialDtor() {
        doNotOptimize(a);
    }
};
} // namespace

// Measures destroy<T>() with a trivial destructor.
static void bench_destroy_trivial() {
    Pool<false> cSrc(sizeof(TrivialDtor), kBlockCount, alignof(TrivialDtor));
    std::vector<TrivialDtor*> cPtrs(kBlockCount);
    for (auto& p : cPtrs)
        p = cSrc.create<TrivialDtor>();
    std::size_t cIndex = 0;

    stdPool sSrc;
    std::vector<TrivialDtor*> sPtrs(kBlockCount);
    for (auto& p : sPtrs) {
        void* raw = sSrc.allocate(sizeof(TrivialDtor), alignof(TrivialDtor));
        p = ::new (raw) TrivialDtor();
    }
    std::size_t sIndex = 0;

    auto c = [&] { cSrc.destroy(cPtrs[cIndex++]); };

    auto s = [&] {
        sPtrs[sIndex]->~TrivialDtor();
        sSrc.deallocate(sPtrs[sIndex], sizeof(TrivialDtor), alignof(TrivialDtor));
        ++sIndex;
    };

    BENCH("destroy<T>() trivial", c, s);
}

// Measures destroy<T>() with a non-trivial destructor.
static void bench_destroy_nontrivial() {
    Pool<false> cSrc(sizeof(NonTrivialDtor), kBlockCount, alignof(NonTrivialDtor));
    std::vector<NonTrivialDtor*> cPtrs(kBlockCount);
    for (auto& p : cPtrs)
        p = cSrc.create<NonTrivialDtor>();
    std::size_t cIndex = 0;

    stdPool sSrc;
    std::vector<NonTrivialDtor*> sPtrs(kBlockCount);
    for (auto& p : sPtrs) {
        void* raw = sSrc.allocate(sizeof(NonTrivialDtor), alignof(NonTrivialDtor));
        p = ::new (raw) NonTrivialDtor();
    }
    std::size_t sIndex = 0;

    auto c = [&] { cSrc.destroy(cPtrs[cIndex++]); };

    auto s = [&] {
        sPtrs[sIndex]->~NonTrivialDtor();
        sSrc.deallocate(sPtrs[sIndex], sizeof(NonTrivialDtor), alignof(NonTrivialDtor));
        ++sIndex;
    };

    BENCH("destroy<T>() non-trivial", c, s);
}

static void run_benchmarks() {
    bench_destroy_trivial();
    std::cout << "\n";

    bench_destroy_nontrivial();
}

REGISTER_BENCH_SUITE();
