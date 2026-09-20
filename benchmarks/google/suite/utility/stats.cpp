// PoolPro Utility Benchmark Suite — Stats
// Measures Pool getStats() performance.
//
// std::pmr::unsynchronized_pool_resource tracks no allocation
// statistics, so this runs solo.
//
// Covers:
// - getStats() on a pool with allocation history

#include <benchmark/benchmark.h>
#include <support/framework.h>

using namespace PoolPro;

namespace {
constexpr std::size_t kBlockSize = sizeof(void*);
constexpr std::size_t kBlockCount = 4096;
} // namespace

// Measures getStats() reading back allocation/deallocation counters.
static void get_stats(benchmark::State& state) {
    Pool<true> pSrc(kBlockSize, kBlockCount);
    for (std::size_t i = 0; i < kBlockCount / 2; ++i)
        (void)pSrc.allocate();

    for (auto _ : state) {
        const auto& stats = pSrc.getStats();
        benchmark::DoNotOptimize(stats.allocations_);
    }
}
BENCHMARK(get_stats);
