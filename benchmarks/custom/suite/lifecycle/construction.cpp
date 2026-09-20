// PoolPro Lifecycle Benchmark Suite — Construction
// Measures Pool construction performance against stdPool,
// the standard library's own pool allocator.
//
// Pool allocates its backing memory eagerly at construction; stdPool
// defers its first internal chunk allocation to the first call to
// allocate(). This benchmarks two genuinely different construction
// strategies, not just two names for the same operation.
//
// Covers:
// - constructing an empty pool sized for a fixed block count

#include <support/framework.h>

using namespace PoolPro;

namespace {
constexpr std::size_t kBlockSize = sizeof(void*);
constexpr std::size_t kBlockCount = 4096;
} // namespace

// Measures constructing an empty pool sized for kBlockCount blocks.
static void bench_construction() {
    auto c = [&] {
        Pool<false> p(kBlockSize, kBlockCount);
        std::size_t n = p.totalBlocks();
        doNotOptimize(n);
    };

    auto s = [&] {
        stdPool p;
        doNotOptimize(&p);
    };

    BENCH("construction", c, s);
}

static void run_benchmarks() {
    bench_construction();
}

REGISTER_BENCH_SUITE();
