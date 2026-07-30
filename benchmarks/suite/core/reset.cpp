// PoolPro Core Benchmark Suite — Reset
// Measures Pool reset() performance against stdPool's release(),
// the standard library's own pool allocator.
//
// Each case starts from a fully allocated pool, so what's measured is
// genuinely reclaiming a populated pool, not an already-empty one.
//
// Covers:
// - reset() alone, immediately reclaiming a full pool
// - reset() followed by refilling to a fixed block count

#include <support/framework.h>

#include <vector>

using namespace PoolPro;

namespace {
constexpr std::size_t kBlockSize = sizeof(void*);
constexpr std::size_t kBlockCount = 4096;
constexpr std::size_t kRefillCount = 1024;
} // namespace

// Measures reset() reclaiming a fully allocated pool, with no refill.
static void bench_reset_alone() {
    Pool<false> cSrc(kBlockSize, kBlockCount);
    std::vector<void*> cScratch(kBlockCount);
    (void)cSrc.allocateBatch(cScratch);

    stdPool sSrc;
    for (std::size_t i = 0; i < kBlockCount; ++i)
        (void)sSrc.allocate(kBlockSize);

    auto c = [&] { cSrc.reset(); };

    auto s = [&] { sSrc.release(); };

    BENCH("reset()", c, s);
}

// Measures reset() followed by refilling to a fixed block count.
static void bench_reset_refill() {
    Pool<false> cSrc(kBlockSize, kBlockCount);
    std::vector<void*> cScratch(kRefillCount);

    stdPool sSrc;

    auto c = [&] {
        cSrc.reset();
        (void)cSrc.allocateBatch(cScratch);
    };

    auto s = [&] {
        sSrc.release();
        for (std::size_t i = 0; i < kRefillCount; ++i)
            (void)sSrc.allocate(kBlockSize);
    };

    BENCH("reset() + refill", c, s);
}

static void run_benchmarks() {
    bench_reset_alone();
    std::cout << "\n";

    bench_reset_refill();
}

REGISTER_BENCH_SUITE();
