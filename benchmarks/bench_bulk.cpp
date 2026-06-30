// Bench Bulk Allocation
// Measures the cost of batch allocation and deallocation
// against equivalent heap allocation and deallocation.
//
// Covers:
// - Batch allocation vs heap allocation loop
// - Batch deallocation vs heap deallocation loop

#include "bench_helper.h"

using namespace AllocatorPro;

static constexpr std::size_t kBatchSize = 16;

// Measures the cost of batch allocation versus repeated heap allocations.
static void bench_batch_allocate() {
    Pool pool{sizeof(Block), kBatchSize};
    void* blocks[kBatchSize]{};

    auto pool_batch_allocate = [&] {
        (void)pool.allocateBatch(blocks);
        doNotOptimize();
        pool.reset();
        doNotOptimize();
    };
    BENCH("pool_batch_allocate", LARGE, pool_batch_allocate);

    auto heap_batch_allocate = [&] {
        for (std::size_t i = 0; i < kBatchSize; ++i)
            blocks[i] = ::operator new(sizeof(Block));
        doNotOptimize();
        for (std::size_t i = 0; i < kBatchSize; ++i)
            ::operator delete(blocks[i]);
        doNotOptimize();
    };
    BENCH("heap_batch_allocate", LARGE, heap_batch_allocate);
}

// Measures the cost of batch deallocation versus repeated heap deallocations.
static void bench_batch_deallocate() {
    Pool pool{sizeof(Block), kBatchSize};
    void* blocks[kBatchSize]{};

    auto pool_batch_deallocate = [&] {
        (void)pool.allocateBatch(blocks);
        doNotOptimize();
        pool.deallocateBatch(blocks);
        doNotOptimize();
    };
    BENCH("pool_batch_deallocate", LARGE, pool_batch_deallocate);

    auto heap_batch_deallocate = [&] {
        for (std::size_t i = 0; i < kBatchSize; ++i)
            blocks[i] = ::operator new(sizeof(Block));
        doNotOptimize();
        for (std::size_t i = 0; i < kBatchSize; ++i)
            ::operator delete(blocks[i]);
        doNotOptimize();
    };
    BENCH("heap_batch_deallocate", LARGE, heap_batch_deallocate);
}

// Executes all bulk allocation benchmark cases.
void run_bulk_benchmarks() {
    setHeader("Bulk Allocation Benchmarks");

    bench_batch_allocate();
    std::cout << "\n";

    bench_batch_deallocate();
    borderLine();
    std::cout << "\n";
}