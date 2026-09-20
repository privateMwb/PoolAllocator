// PoolPro Core Benchmark Suite — Deallocate
// Measures Pool deallocate() performance against stdPool,
// the standard library's own pool allocator.
//
// Each case pre-allocates a block count sized generously above the
// LARGE iteration tier before timing starts, so the timed lambda only
// ever consumes an already-allocated pointer — allocation cost is
// never part of what's measured.
//
// Covers:
// - deallocate() returning a block to an otherwise full pool

#include <support/framework.h>

#include <vector>

using namespace PoolPro;

namespace {
constexpr std::size_t kBlockSize = sizeof(void*);
constexpr std::size_t kBlockCount = 2'000'000;
} // namespace

// Measures deallocate() releasing a block back to the pool.
static void bench_deallocate() {
    Pool<false> cSrc(kBlockSize, kBlockCount);
    std::vector<void*> cPtrs(kBlockCount);
    (void)cSrc.allocateBatch(cPtrs);
    std::size_t cIndex = 0;

    stdPool sSrc;
    std::vector<void*> sPtrs(kBlockCount);
    for (auto& p : sPtrs)
        p = sSrc.allocate(kBlockSize);
    std::size_t sIndex = 0;

    auto c = [&] { cSrc.deallocate(cPtrs[cIndex++]); };

    auto s = [&] {
        sSrc.deallocate(sPtrs[sIndex], kBlockSize);
        ++sIndex;
    };

    BENCH("deallocate()", c, s);
}

static void run_benchmarks() {
    bench_deallocate();
}

REGISTER_BENCH_SUITE();
