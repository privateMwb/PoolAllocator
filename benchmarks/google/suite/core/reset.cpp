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

#include <benchmark/benchmark.h>
#include <support/framework.h>

#include <vector>

using namespace PoolPro;

namespace {
constexpr std::size_t kBlockSize = sizeof(void*);
constexpr std::size_t kBlockCount = 4096;
constexpr std::size_t kRefillCount = 1024;
} // namespace

// Measures reset() reclaiming a fully allocated pool, with no refill.
static void reset_alone_pool(benchmark::State& state) {
    Pool<false> cSrc(kBlockSize, kBlockCount);
    std::vector<void*> cScratch(kBlockCount);
    (void)cSrc.allocateBatch(cScratch);

    for (auto _ : state) {
        cSrc.reset();
    }
}
BENCHMARK(reset_alone_pool);

// stdPool counterpart of reset_alone_pool, using release().
static void reset_alone_std(benchmark::State& state) {
    stdPool sSrc;
    for (std::size_t i = 0; i < kBlockCount; ++i)
        (void)sSrc.allocate(kBlockSize);

    for (auto _ : state) {
        sSrc.release();
    }
}
BENCHMARK(reset_alone_std);

// Measures reset() followed by refilling to a fixed block count.
static void reset_refill_pool(benchmark::State& state) {
    Pool<false> cSrc(kBlockSize, kBlockCount);
    std::vector<void*> cScratch(kRefillCount);

    for (auto _ : state) {
        cSrc.reset();
        (void)cSrc.allocateBatch(cScratch);
    }
}
BENCHMARK(reset_refill_pool);

// stdPool counterpart of reset_refill_pool, using release().
static void reset_refill_std(benchmark::State& state) {
    stdPool sSrc;

    for (auto _ : state) {
        sSrc.release();
        for (std::size_t i = 0; i < kRefillCount; ++i)
            (void)sSrc.allocate(kBlockSize);
    }
}
BENCHMARK(reset_refill_std);
