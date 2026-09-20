// PoolPro Scaling Benchmark Suite — Pool Size
// Measures Pool allocate() performance against stdPool, the standard
// library's own pool allocator, as the pool's total block count grows
// — a separate axis from the iteration count, which repeats the same
// fixed-size operation more times rather than growing the pool itself.
//
// Each case wraps around at the same cadence on both sides: Pool via
// reset() (O(1), regardless of how full it was) whenever it runs out
// — keeping the steady-state allocation path under measurement instead
// of degrading into the failure path, which is exhaustion.cpp's
// concern, not this one's — and stdPool via periodic release() every
// matching block count, so its unbounded default upstream never
// accumulates more resident memory than Pool's own footprint.
//
// Covers:
// - allocate() from a small pool (1K blocks)
// - allocate() from a medium pool (100K blocks)
// - allocate() from a large pool (10M blocks)

#include <benchmark/benchmark.h>
#include <support/framework.h>

using namespace PoolPro;

namespace {
constexpr std::size_t kBlockSize = sizeof(void*);
constexpr std::size_t kSmallPool = 1'000;
constexpr std::size_t kMediumPool = 100'000;
constexpr std::size_t kLargePool = 10'000'000;

// Allocates from `pool`, wrapping around via reset() if it's exhausted.
void* allocateWrapping(Pool<false>& pool) {
    void* p = pool.allocate();
    if (!p) [[unlikely]] {
        pool.reset();
        p = pool.allocate();
    }
    return p;
}

// Allocates from `pool`, releasing everything back to its upstream
// every `period` calls so it never outgrows Pool's own footprint.
void* allocateStdWrapping(stdPool& pool, std::size_t size, std::size_t period,
                          std::size_t& counter) {
    if (counter == period) [[unlikely]] {
        pool.release();
        counter = 0;
    }
    ++counter;
    return pool.allocate(size);
}
} // namespace

// Measures allocate() from a small (1K-block) pool.
static void pool_size_small_pool(benchmark::State& state) {
    Pool<false> cSrc(kBlockSize, kSmallPool);

    for (auto _ : state) {
        void* p = allocateWrapping(cSrc);
        benchmark::DoNotOptimize(p);
    }
}
BENCHMARK(pool_size_small_pool);

// stdPool counterpart of pool_size_small_pool.
static void pool_size_small_std(benchmark::State& state) {
    stdPool sSrc;
    std::size_t sCounter = 0;

    for (auto _ : state) {
        void* p = allocateStdWrapping(sSrc, kBlockSize, kSmallPool, sCounter);
        benchmark::DoNotOptimize(p);
    }
}
BENCHMARK(pool_size_small_std);

// Measures allocate() from a medium (100K-block) pool.
static void pool_size_medium_pool(benchmark::State& state) {
    Pool<false> cSrc(kBlockSize, kMediumPool);

    for (auto _ : state) {
        void* p = allocateWrapping(cSrc);
        benchmark::DoNotOptimize(p);
    }
}
BENCHMARK(pool_size_medium_pool);

// stdPool counterpart of pool_size_medium_pool.
static void pool_size_medium_std(benchmark::State& state) {
    stdPool sSrc;
    std::size_t sCounter = 0;

    for (auto _ : state) {
        void* p = allocateStdWrapping(sSrc, kBlockSize, kMediumPool, sCounter);
        benchmark::DoNotOptimize(p);
    }
}
BENCHMARK(pool_size_medium_std);

// Measures allocate() from a large (10M-block) pool.
static void pool_size_large_pool(benchmark::State& state) {
    Pool<false> cSrc(kBlockSize, kLargePool);

    for (auto _ : state) {
        void* p = allocateWrapping(cSrc);
        benchmark::DoNotOptimize(p);
    }
}
BENCHMARK(pool_size_large_pool);

// stdPool counterpart of pool_size_large_pool.
static void pool_size_large_std(benchmark::State& state) {
    stdPool sSrc;
    std::size_t sCounter = 0;

    for (auto _ : state) {
        void* p = allocateStdWrapping(sSrc, kBlockSize, kLargePool, sCounter);
        benchmark::DoNotOptimize(p);
    }
}
BENCHMARK(pool_size_large_std);
