// PoolPro Core Benchmark Suite — Deallocate
// Measures Pool deallocate() performance against stdPool,
// the standard library's own pool allocator.
//
// Each case pre-allocates a block count sized generously above the
// fixed iteration count (kIterations) before timing starts, so the
// timed loop only ever consumes an already-allocated pointer —
// allocation cost is never part of what's measured.
//
// Covers:
// - deallocate() returning a block to an otherwise full pool

#include <benchmark/benchmark.h>
#include <support/framework.h>

#include <vector>

using namespace PoolPro;

namespace {
constexpr std::size_t kBlockSize = sizeof(void*);
constexpr std::size_t kBlockCount = 2'000'000;
constexpr std::size_t kIterations = 1'000'000;
static_assert(kIterations <= kBlockCount, "pre-allocated pointers must not run out mid-benchmark");
} // namespace

// Measures deallocate() releasing a block back to the pool.
static void deallocate_pool(benchmark::State& state) {
    Pool<false> cSrc(kBlockSize, kBlockCount);
    std::vector<void*> cPtrs(kBlockCount);
    (void)cSrc.allocateBatch(cPtrs);
    std::size_t cIndex = 0;

    for (auto _ : state) {
        cSrc.deallocate(cPtrs[cIndex++]);
    }
}
BENCHMARK(deallocate_pool)->Iterations(kIterations);

// stdPool counterpart of deallocate_pool.
static void deallocate_std(benchmark::State& state) {
    stdPool sSrc;
    std::vector<void*> sPtrs(kBlockCount);
    for (auto& p : sPtrs)
        p = sSrc.allocate(kBlockSize);
    std::size_t sIndex = 0;

    for (auto _ : state) {
        sSrc.deallocate(sPtrs[sIndex], kBlockSize);
        ++sIndex;
    }
}
BENCHMARK(deallocate_std)->Iterations(kIterations);
