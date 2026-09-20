// PoolPro Core Benchmark Suite — Allocate
// Measures Pool allocate() performance against stdPool,
// the standard library's own pool allocator.
//
// Each pool owns a block count sized generously above the fixed
// iteration count (kIterations), so repeated calls never exhaust it
// mid-benchmark — only the steady-state allocation path is measured,
// with no branching to the failure path.
//
// Covers:
// - allocate() from a pool of small, word-sized blocks at the default alignment
// - allocate() from a pool of larger, cache-line-sized blocks at the default alignment
// - allocate() from a pool of small blocks at an over-aligned boundary (64 bytes)

#include <benchmark/benchmark.h>
#include <support/framework.h>

using namespace PoolPro;

namespace {
constexpr std::size_t kBlockCount = 2'000'000;
constexpr std::size_t kIterations = 1'000'000;
static_assert(kIterations <= kBlockCount, "pool must not be exhausted mid-benchmark");

constexpr std::size_t kSmallSize = sizeof(void*);
constexpr std::size_t kLargeSize = 256;
constexpr std::size_t kOverAlignment = 64;
} // namespace

// Measures allocate() from a pool of small, word-sized blocks at the
// default alignment — the cheapest possible call through the hot path.
static void allocate_small_pool(benchmark::State& state) {
    Pool<false> cSrc(kSmallSize, kBlockCount);

    for (auto _ : state) {
        void* p = cSrc.allocate();
        benchmark::DoNotOptimize(p);
    }
}
BENCHMARK(allocate_small_pool)->Iterations(kIterations);

// stdPool counterpart of allocate_small_pool.
static void allocate_small_std(benchmark::State& state) {
    stdPool sSrc;

    for (auto _ : state) {
        void* p = sSrc.allocate(kSmallSize);
        benchmark::DoNotOptimize(p);
    }
}
BENCHMARK(allocate_small_std)->Iterations(kIterations);

// Measures allocate() from a pool of larger, cache-line-sized blocks at
// the default alignment.
static void allocate_large_pool(benchmark::State& state) {
    Pool<false> cSrc(kLargeSize, kBlockCount);

    for (auto _ : state) {
        void* p = cSrc.allocate();
        benchmark::DoNotOptimize(p);
    }
}
BENCHMARK(allocate_large_pool)->Iterations(kIterations);

// stdPool counterpart of allocate_large_pool.
static void allocate_large_std(benchmark::State& state) {
    stdPool sSrc;

    for (auto _ : state) {
        void* p = sSrc.allocate(kLargeSize);
        benchmark::DoNotOptimize(p);
    }
}
BENCHMARK(allocate_large_std)->Iterations(kIterations);

// Measures allocate() from a pool of small blocks at an over-aligned
// (64-byte) boundary.
static void allocate_aligned_pool(benchmark::State& state) {
    Pool<false> cSrc(kSmallSize, kBlockCount, kOverAlignment);

    for (auto _ : state) {
        void* p = cSrc.allocate();
        benchmark::DoNotOptimize(p);
    }
}
BENCHMARK(allocate_aligned_pool)->Iterations(kIterations);

// stdPool counterpart of allocate_aligned_pool.
static void allocate_aligned_std(benchmark::State& state) {
    stdPool sSrc;

    for (auto _ : state) {
        void* p = sSrc.allocate(kSmallSize, kOverAlignment);
        benchmark::DoNotOptimize(p);
    }
}
BENCHMARK(allocate_aligned_std)->Iterations(kIterations);
