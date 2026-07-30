// PoolPro Access Benchmark Suite — State Query
// Measures Pool introspection performance.
//
// std::pmr::unsynchronized_pool_resource exposes none of these
// queries, so every case runs solo.
//
// Covers:
// - usedBlocks(), freeBlocks(), totalBlocks(), capacity(), blockStride()

#include <support/framework.h>

using namespace PoolPro;

namespace {
constexpr std::size_t kBlockSize = sizeof(void*);
constexpr std::size_t kBlockCount = 4096;
} // namespace

// Measures usedBlocks().
static void bench_used_blocks() {
    Pool<false> pSrc(kBlockSize, kBlockCount);
    (void)pSrc.allocate();

    auto a = [&] {
        std::size_t v = pSrc.usedBlocks();
        doNotOptimize(v);
    };

    BENCH_SOLO("usedBlocks()", a);
}

// Measures freeBlocks().
static void bench_free_blocks() {
    Pool<false> pSrc(kBlockSize, kBlockCount);
    (void)pSrc.allocate();

    auto a = [&] {
        std::size_t v = pSrc.freeBlocks();
        doNotOptimize(v);
    };

    BENCH_SOLO("freeBlocks()", a);
}

// Measures totalBlocks().
static void bench_total_blocks() {
    Pool<false> pSrc(kBlockSize, kBlockCount);

    auto a = [&] {
        std::size_t v = pSrc.totalBlocks();
        doNotOptimize(v);
    };

    BENCH_SOLO("totalBlocks()", a);
}

// Measures capacity().
static void bench_capacity() {
    Pool<false> pSrc(kBlockSize, kBlockCount);

    auto a = [&] {
        std::size_t v = pSrc.capacity();
        doNotOptimize(v);
    };

    BENCH_SOLO("capacity()", a);
}

// Measures blockStride().
static void bench_block_stride() {
    Pool<false> pSrc(kBlockSize, kBlockCount);

    auto a = [&] {
        std::size_t v = pSrc.blockStride();
        doNotOptimize(v);
    };

    BENCH_SOLO("blockStride()", a);
}

static void run_benchmarks() {
    bench_used_blocks();
    std::cout << "\n";

    bench_free_blocks();
    std::cout << "\n";

    bench_total_blocks();
    std::cout << "\n";

    bench_capacity();
    std::cout << "\n";

    bench_block_stride();
}

REGISTER_BENCH_SUITE();
