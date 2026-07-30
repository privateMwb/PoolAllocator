// PoolPro Core Benchmark Suite — Batch Deallocate
// Measures Pool deallocateBatch() performance.
//
// Pre-allocates a block count sized generously above what the LARGE
// iteration tier could consume at the batch size below, before timing
// starts, so the timed lambda only ever releases already-allocated
// pointers.
// std::pmr::unsynchronized_pool_resource has no batch deallocation
// entry point, so this runs solo.
//
// Covers:
// - deallocateBatch() returning a fixed-size batch to the free list

#include <support/framework.h>

#include <span>
#include <vector>

using namespace PoolPro;

namespace {
constexpr std::size_t kBlockSize = sizeof(void*);
constexpr std::size_t kBatchSize = 8;
constexpr std::size_t kBlockCount = 10'000'000;
} // namespace

// Measures deallocateBatch() releasing a fixed-size batch.
static void bench_batch_deallocate() {
    Pool<false> cSrc(kBlockSize, kBlockCount);
    std::vector<void*> ptrs(kBlockCount);
    (void)cSrc.allocateBatch(ptrs);
    std::size_t index = 0;

    auto a = [&] {
        std::span<void*> batch(ptrs.data() + index, kBatchSize);
        cSrc.deallocateBatch(batch);
        index += kBatchSize;
    };

    BENCH_SOLO("deallocateBatch()", a);
}

static void run_benchmarks() {
    bench_batch_deallocate();
}

REGISTER_BENCH_SUITE();
