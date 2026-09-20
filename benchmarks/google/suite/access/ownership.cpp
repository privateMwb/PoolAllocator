// PoolPro Access Benchmark Suite — Ownership
// Measures Pool owns() performance.
//
// std::pmr::unsynchronized_pool_resource exposes no ownership query,
// so both cases run solo.
//
// Covers:
// - owns() on a pointer that belongs to the pool (hit path)
// - owns() on a pointer that does not belong to the pool (miss path)

#include <benchmark/benchmark.h>
#include <support/framework.h>

using namespace PoolPro;

namespace {
constexpr std::size_t kBlockSize = sizeof(void*);
constexpr std::size_t kBlockCount = 1024;
} // namespace

// Measures owns() on a pointer that belongs to the pool.
static void owns_hit(benchmark::State& state) {
    Pool<false> pSrc(kBlockSize, kBlockCount);
    void* ptr = pSrc.allocate();

    for (auto _ : state) {
        bool v = pSrc.owns(ptr);
        benchmark::DoNotOptimize(v);
    }
}
BENCHMARK(owns_hit);

// Measures owns() on a pointer that does not belong to the pool.
static void owns_miss(benchmark::State& state) {
    Pool<false> pSrc(kBlockSize, kBlockCount);
    (void)pSrc.allocate();

    int outside = 0;
    const void* ptr = &outside;

    for (auto _ : state) {
        bool v = pSrc.owns(ptr);
        benchmark::DoNotOptimize(v);
    }
}
BENCHMARK(owns_miss);
