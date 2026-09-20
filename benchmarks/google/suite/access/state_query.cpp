// PoolPro Access Benchmark Suite — State Query
// Measures Pool introspection performance.
//
// std::pmr::unsynchronized_pool_resource exposes none of these
// queries, so every case runs solo.
//
// Covers:
// - usedBlocks(), freeBlocks(), totalBlocks(), capacity(), blockStride()

#include <benchmark/benchmark.h>
#include <support/framework.h>

using namespace PoolPro;

namespace {
constexpr std::size_t kBlockSize = sizeof(void*);
constexpr std::size_t kBlockCount = 4096;
} // namespace

// Measures usedBlocks().
static void used_blocks(benchmark::State& state) {
    Pool<false> pSrc(kBlockSize, kBlockCount);
    (void)pSrc.allocate();

    for (auto _ : state) {
        std::size_t v = pSrc.usedBlocks();
        benchmark::DoNotOptimize(v);
    }
}
BENCHMARK(used_blocks);

// Measures freeBlocks().
static void free_blocks(benchmark::State& state) {
    Pool<false> pSrc(kBlockSize, kBlockCount);
    (void)pSrc.allocate();

    for (auto _ : state) {
        std::size_t v = pSrc.freeBlocks();
        benchmark::DoNotOptimize(v);
    }
}
BENCHMARK(free_blocks);

// Measures totalBlocks().
static void total_blocks(benchmark::State& state) {
    Pool<false> pSrc(kBlockSize, kBlockCount);

    for (auto _ : state) {
        std::size_t v = pSrc.totalBlocks();
        benchmark::DoNotOptimize(v);
    }
}
BENCHMARK(total_blocks);

// Measures capacity().
static void capacity(benchmark::State& state) {
    Pool<false> pSrc(kBlockSize, kBlockCount);

    for (auto _ : state) {
        std::size_t v = pSrc.capacity();
        benchmark::DoNotOptimize(v);
    }
}
BENCHMARK(capacity);

// Measures blockStride().
static void block_stride(benchmark::State& state) {
    Pool<false> pSrc(kBlockSize, kBlockCount);

    for (auto _ : state) {
        std::size_t v = pSrc.blockStride();
        benchmark::DoNotOptimize(v);
    }
}
BENCHMARK(block_stride);
