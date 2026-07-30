// PoolPro Access Benchmark Suite — Ownership
// Measures Pool owns() performance.
//
// std::pmr::unsynchronized_pool_resource exposes no ownership query,
// so both cases run solo.
//
// Covers:
// - owns() on a pointer that belongs to the pool (hit path)
// - owns() on a pointer that does not belong to the pool (miss path)

#include <support/framework.h>

using namespace PoolPro;

namespace {
constexpr std::size_t kBlockSize = sizeof(void*);
constexpr std::size_t kBlockCount = 1024;
} // namespace

// Measures owns() on a pointer that belongs to the pool.
static void bench_owns_hit() {
    Pool<false> pSrc(kBlockSize, kBlockCount);
    void* ptr = pSrc.allocate();

    auto a = [&] {
        bool v = pSrc.owns(ptr);
        doNotOptimize(v);
    };

    BENCH_SOLO("owns() hit", a);
}

// Measures owns() on a pointer that does not belong to the pool.
static void bench_owns_miss() {
    Pool<false> pSrc(kBlockSize, kBlockCount);
    (void)pSrc.allocate();

    int outside = 0;
    const void* ptr = &outside;

    auto a = [&] {
        bool v = pSrc.owns(ptr);
        doNotOptimize(v);
    };

    BENCH_SOLO("owns() miss", a);
}

static void run_benchmarks() {
    bench_owns_hit();
    std::cout << "\n";

    bench_owns_miss();
}

REGISTER_BENCH_SUITE();
