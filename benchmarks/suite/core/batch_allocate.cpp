// PoolPro Core Benchmark Suite — Batch Allocate
// Measures Pool allocateBatch() performance.
//
// The pool owns a block count sized generously above what the LARGE
// iteration tier could consume at the batch size below, so repeated
// calls never exhaust it mid-benchmark.
// std::pmr::unsynchronized_pool_resource has no batch allocation entry
// point, so this runs solo.
//
// Covers:
// - allocateBatch() filling a fixed-size batch from the free list and
//   virgin memory

#include <support/framework.h>

#include <vector>

using namespace PoolPro;

namespace {
constexpr std::size_t kBlockSize = sizeof(void*);
constexpr std::size_t kBatchSize = 8;
constexpr std::size_t kBlockCount = 10'000'000;
} // namespace

// Measures allocateBatch() filling a fixed-size batch.
static void bench_batch_allocate() {
    Pool<false> cSrc(kBlockSize, kBlockCount);
    std::vector<void*> batch(kBatchSize);

    auto a = [&] {
        std::size_t n = cSrc.allocateBatch(batch);
        doNotOptimize(n);
    };

    BENCH_SOLO("allocateBatch()", a);
}

static void run_benchmarks() {
    bench_batch_allocate();
}

REGISTER_BENCH_SUITE();
