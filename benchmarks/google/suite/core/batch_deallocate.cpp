// PoolPro Core Benchmark Suite — Batch Deallocate
// Measures Pool deallocateBatch() performance.
//
// Pre-allocates a block count sized generously above what the fixed
// iteration count (kIterations) could consume at the batch size below,
// before timing starts, so the timed loop only ever releases
// already-allocated pointers.
// std::pmr::unsynchronized_pool_resource has no batch deallocation
// entry point, so this runs solo.
//
// Covers:
// - deallocateBatch() returning a fixed-size batch to the free list

#include <benchmark/benchmark.h>
#include <support/framework.h>

#include <span>
#include <vector>

using namespace PoolPro;

namespace {
constexpr std::size_t kBlockSize = sizeof(void*);
constexpr std::size_t kBatchSize = 8;
constexpr std::size_t kBlockCount = 10'000'000;
constexpr std::size_t kIterations = 1'000'000;
static_assert(kIterations * kBatchSize <= kBlockCount,
              "pre-allocated pointers must not run out mid-benchmark");
} // namespace

// Measures deallocateBatch() releasing a fixed-size batch.
static void batch_deallocate(benchmark::State& state) {
    Pool<false> cSrc(kBlockSize, kBlockCount);
    std::vector<void*> ptrs(kBlockCount);
    (void)cSrc.allocateBatch(ptrs);
    std::size_t index = 0;

    for (auto _ : state) {
        std::span<void*> batch(ptrs.data() + index, kBatchSize);
        cSrc.deallocateBatch(batch);
        index += kBatchSize;
    }
}
BENCHMARK(batch_deallocate)->Iterations(kIterations);
