// PoolPro Lifecycle Benchmark Suite — Construction
// Measures Pool construction performance against stdPool,
// the standard library's own pool allocator.
//
// Pool allocates its backing memory eagerly at construction; stdPool
// defers its first internal chunk allocation to the first call to
// allocate(). This benchmarks two genuinely different construction
// strategies, not just two names for the same operation.
//
// Covers:
// - constructing an empty pool sized for a fixed block count

#include <benchmark/benchmark.h>
#include <support/framework.h>

using namespace PoolPro;

namespace {
constexpr std::size_t kBlockSize = sizeof(void*);
constexpr std::size_t kBlockCount = 4096;
} // namespace

// Measures constructing an empty pool sized for kBlockCount blocks.
static void construction_pool(benchmark::State& state) {
    for (auto _ : state) {
        Pool<false> p(kBlockSize, kBlockCount);
        std::size_t n = p.totalBlocks();
        benchmark::DoNotOptimize(n);
    }
}
BENCHMARK(construction_pool);

// stdPool counterpart of construction_pool.
static void construction_std(benchmark::State& state) {
    for (auto _ : state) {
        stdPool p;
        benchmark::DoNotOptimize(&p);
    }
}
BENCHMARK(construction_std);
