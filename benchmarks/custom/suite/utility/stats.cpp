// PoolPro Utility Benchmark Suite — Stats
// Measures Pool getStats() performance.
//
// std::pmr::unsynchronized_pool_resource tracks no allocation
// statistics, so this runs solo.
//
// Covers:
// - getStats() on a pool with allocation history

#include <support/framework.h>

using namespace PoolPro;

namespace {
constexpr std::size_t kBlockSize = sizeof(void*);
constexpr std::size_t kBlockCount = 4096;
} // namespace

// Measures getStats() reading back allocation/deallocation counters.
static void bench_get_stats() {
    Pool<true> pSrc(kBlockSize, kBlockCount);
    for (std::size_t i = 0; i < kBlockCount / 2; ++i)
        (void)pSrc.allocate();

    auto a = [&] {
        const auto& stats = pSrc.getStats();
        doNotOptimize(stats.allocations_);
    };

    BENCH_SOLO("getStats()", a);
}

static void run_benchmarks() {
    bench_get_stats();
}

REGISTER_BENCH_SUITE();
