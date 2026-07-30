// PoolPro Scaling Benchmark Suite — Pool Size
// Measures Pool allocate() performance against stdPool, the standard
// library's own pool allocator, as the pool's total block count grows
// — a separate axis from the SMALL/MEDIUM/LARGE iteration tiers, which
// repeat the same fixed-size operation more times rather than growing
// the pool itself.
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
static void bench_pool_size_small() {
    Pool<false> cSrc(kBlockSize, kSmallPool);
    stdPool sSrc;
    std::size_t sCounter = 0;

    auto c = [&] {
        void* p = allocateWrapping(cSrc);
        doNotOptimize(p);
    };

    auto s = [&] {
        void* p = allocateStdWrapping(sSrc, kBlockSize, kSmallPool, sCounter);
        doNotOptimize(p);
    };

    BENCH("allocate() 1K-block pool", c, s);
}

// Measures allocate() from a medium (100K-block) pool.
static void bench_pool_size_medium() {
    Pool<false> cSrc(kBlockSize, kMediumPool);
    stdPool sSrc;
    std::size_t sCounter = 0;

    auto c = [&] {
        void* p = allocateWrapping(cSrc);
        doNotOptimize(p);
    };

    auto s = [&] {
        void* p = allocateStdWrapping(sSrc, kBlockSize, kMediumPool, sCounter);
        doNotOptimize(p);
    };

    BENCH("allocate() 100K-block pool", c, s);
}

// Measures allocate() from a large (10M-block) pool.
static void bench_pool_size_large() {
    Pool<false> cSrc(kBlockSize, kLargePool);
    stdPool sSrc;
    std::size_t sCounter = 0;

    auto c = [&] {
        void* p = allocateWrapping(cSrc);
        doNotOptimize(p);
    };

    auto s = [&] {
        void* p = allocateStdWrapping(sSrc, kBlockSize, kLargePool, sCounter);
        doNotOptimize(p);
    };

    BENCH("allocate() 10M-block pool", c, s);
}

static void run_benchmarks() {
    bench_pool_size_small();
    std::cout << "\n";

    bench_pool_size_medium();
    std::cout << "\n";

    bench_pool_size_large();
}

REGISTER_BENCH_SUITE();
