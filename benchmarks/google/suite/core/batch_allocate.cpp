// PoolPro Core Benchmark Suite — Batch Allocate
// Measures Pool allocateBatch() performance.
//
// The pool owns a block count sized generously above what the fixed
// iteration count (kIterations) could consume at the batch size below,
// so repeated calls never exhaust it mid-benchmark.
// std::pmr::unsynchronized_pool_resource has no batch allocation entry
// point, so this runs solo.
//
// Covers:
// - allocateBatch() filling a fixed-size batch from the free list and
//   virgin memory

#include <benchmark/benchmark.h>
#include <support/framework.h>

#include <vector>

using namespace PoolPro;

namespace {
constexpr std::size_t kBlockSize = sizeof(void*);
constexpr std::size_t kBatchSize = 8;
constexpr std::size_t kBlockCount = 10'000'000;
constexpr std::size_t kIterations = 1'000'000;
static_assert(kIterations * kBatchSize <= kBlockCount, "pool must not be exhausted mid-benchmark");
} // namespace

// Measures allocateBatch() filling a fixed-size batch.
static void batch_allocate(benchmark::State& state) {
    Pool<false> cSrc(kBlockSize, kBlockCount);
    std::vector<void*> batch(kBatchSize);

    for (auto _ : state) {
        std::size_t n = cSrc.allocateBatch(batch);
        benchmark::DoNotOptimize(n);
    }
}
BENCHMARK(batch_allocate)->Iterations(kIterations);
