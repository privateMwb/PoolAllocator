// PoolPro Core Benchmark Suite — Allocate
// Measures Pool allocate() performance against stdPool,
// the standard library's own pool allocator.
//
// Each pool owns a block count sized generously above the LARGE
// iteration tier, so repeated calls never exhaust it mid-benchmark —
// only the steady-state allocation path is measured, with no branching
// to the failure path.
//
// Covers:
// - allocate() from a pool of small, word-sized blocks at the default alignment
// - allocate() from a pool of larger, cache-line-sized blocks at the default alignment
// - allocate() from a pool of small blocks at an over-aligned boundary (64 bytes)

#include <support/framework.h>

using namespace PoolPro;

namespace {
constexpr std::size_t kBlockCount = 2'000'000;
constexpr std::size_t kSmallSize = sizeof(void*);
constexpr std::size_t kLargeSize = 256;
constexpr std::size_t kOverAlignment = 64;
} // namespace

// Measures allocate() from a pool of small, word-sized blocks at the
// default alignment — the cheapest possible call through the hot path.
static void bench_allocate_small() {
    Pool<false> cSrc(kSmallSize, kBlockCount);
    stdPool sSrc;

    auto c = [&] {
        void* p = cSrc.allocate();
        doNotOptimize(p);
    };

    auto s = [&] {
        void* p = sSrc.allocate(kSmallSize);
        doNotOptimize(p);
    };

    BENCH("allocate() small", c, s);
}

// Measures allocate() from a pool of larger, cache-line-sized blocks at
// the default alignment.
static void bench_allocate_large() {
    Pool<false> cSrc(kLargeSize, kBlockCount);
    stdPool sSrc;

    auto c = [&] {
        void* p = cSrc.allocate();
        doNotOptimize(p);
    };

    auto s = [&] {
        void* p = sSrc.allocate(kLargeSize);
        doNotOptimize(p);
    };

    BENCH("allocate() large", c, s);
}

// Measures allocate() from a pool of small blocks at an over-aligned
// (64-byte) boundary.
static void bench_allocate_aligned() {
    Pool<false> cSrc(kSmallSize, kBlockCount, kOverAlignment);
    stdPool sSrc;

    auto c = [&] {
        void* p = cSrc.allocate();
        doNotOptimize(p);
    };

    auto s = [&] {
        void* p = sSrc.allocate(kSmallSize, kOverAlignment);
        doNotOptimize(p);
    };

    BENCH("allocate() over-aligned", c, s);
}

// Executes all allocate benchmark cases.
static void run_benchmarks() {
    bench_allocate_small();
    std::cout << "\n";

    bench_allocate_large();
    std::cout << "\n";

    bench_allocate_aligned();
}

REGISTER_BENCH_SUITE();
