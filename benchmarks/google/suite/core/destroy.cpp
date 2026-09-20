// PoolPro Core Benchmark Suite — Destroy
// Measures Pool destroy<T>() performance against stdPool,
// the standard library's own pool allocator, with placement new.
//
// Each case pre-constructs a block count sized generously above the
// fixed iteration count (kIterations) before timing starts, so the
// timed loop only ever destroys an already-constructed object.
//
// Covers:
// - destroy<T>() with a trivial destructor
// - destroy<T>() with a non-trivial destructor

#include <benchmark/benchmark.h>
#include <support/framework.h>

#include <vector>

using namespace PoolPro;

namespace {
constexpr std::size_t kBlockCount = 2'000'000;
constexpr std::size_t kIterations = 1'000'000;
static_assert(kIterations <= kBlockCount, "pre-constructed objects must not run out mid-benchmark");

struct TrivialDtor {
    // Padded to sizeof(void*) so it satisfies Pool's minimum block size —
    // a free block must be able to hold a FreeNode.
    void* a = nullptr;
};

struct NonTrivialDtor {
    void* a = nullptr;
    ~NonTrivialDtor() {
        benchmark::DoNotOptimize(a);
    }
};
} // namespace

// Measures destroy<T>() with a trivial destructor.
static void destroy_trivial_pool(benchmark::State& state) {
    Pool<false> cSrc(sizeof(TrivialDtor), kBlockCount, alignof(TrivialDtor));
    std::vector<TrivialDtor*> cPtrs(kBlockCount);
    for (auto& p : cPtrs)
        p = cSrc.create<TrivialDtor>();
    std::size_t cIndex = 0;

    for (auto _ : state) {
        cSrc.destroy(cPtrs[cIndex++]);
    }
}
BENCHMARK(destroy_trivial_pool)->Iterations(kIterations);

// stdPool counterpart of destroy_trivial_pool.
static void destroy_trivial_std(benchmark::State& state) {
    stdPool sSrc;
    std::vector<TrivialDtor*> sPtrs(kBlockCount);
    for (auto& p : sPtrs) {
        void* raw = sSrc.allocate(sizeof(TrivialDtor), alignof(TrivialDtor));
        p = ::new (raw) TrivialDtor();
    }
    std::size_t sIndex = 0;

    for (auto _ : state) {
        sPtrs[sIndex]->~TrivialDtor();
        sSrc.deallocate(sPtrs[sIndex], sizeof(TrivialDtor), alignof(TrivialDtor));
        ++sIndex;
    }
}
BENCHMARK(destroy_trivial_std)->Iterations(kIterations);

// Measures destroy<T>() with a non-trivial destructor.
static void destroy_nontrivial_pool(benchmark::State& state) {
    Pool<false> cSrc(sizeof(NonTrivialDtor), kBlockCount, alignof(NonTrivialDtor));
    std::vector<NonTrivialDtor*> cPtrs(kBlockCount);
    for (auto& p : cPtrs)
        p = cSrc.create<NonTrivialDtor>();
    std::size_t cIndex = 0;

    for (auto _ : state) {
        cSrc.destroy(cPtrs[cIndex++]);
    }
}
BENCHMARK(destroy_nontrivial_pool)->Iterations(kIterations);

// stdPool counterpart of destroy_nontrivial_pool.
static void destroy_nontrivial_std(benchmark::State& state) {
    stdPool sSrc;
    std::vector<NonTrivialDtor*> sPtrs(kBlockCount);
    for (auto& p : sPtrs) {
        void* raw = sSrc.allocate(sizeof(NonTrivialDtor), alignof(NonTrivialDtor));
        p = ::new (raw) NonTrivialDtor();
    }
    std::size_t sIndex = 0;

    for (auto _ : state) {
        sPtrs[sIndex]->~NonTrivialDtor();
        sSrc.deallocate(sPtrs[sIndex], sizeof(NonTrivialDtor), alignof(NonTrivialDtor));
        ++sIndex;
    }
}
BENCHMARK(destroy_nontrivial_std)->Iterations(kIterations);
