// Bench Reset
// Measures the cost of pool reset against manual deallocation loop
// and heap equivalent.
//
// Covers:
// - pool reset vs manual deallocate loop
// - pool reset vs heap delete loop

#include "bench_helper.h"

using namespace AllocatorPro;

static constexpr std::size_t kResetPoolSize = 256;

// Reset
// measures pool reset vs manual deallocate loop
static void bench_reset() {
    Pool pool{sizeof(Block), kResetPoolSize};
    void* blocks[kResetPoolSize]{};

    auto pool_reset = [&] {
        (void)pool.allocateBatch(blocks);
        doNotOptimize();
        pool.reset();
        doNotOptimize();
    };
    BENCH("pool_reset", LARGE, pool_reset);

    auto pool_manual_deallocate = [&] {
        (void)pool.allocateBatch(blocks);
        doNotOptimize();
        pool.deallocateBatch(blocks);
        doNotOptimize();
    };
    BENCH("pool_manual_deallocate", LARGE, pool_manual_deallocate);

    auto heap_reset = [&] {
        for (std::size_t i = 0; i < kResetPoolSize; ++i)
            blocks[i] = ::operator new(sizeof(Block));
        doNotOptimize();
        for (std::size_t i = 0; i < kResetPoolSize; ++i)
            ::operator delete(blocks[i]);
        doNotOptimize();
    };
    BENCH("heap_reset", LARGE, heap_reset);
}

// Benchmark Runner
// Executes all reset benchmark cases.
void run_reset_benchmarks() {
    setHeader("Reset Benchmarks");

    bench_reset();
    borderLine();
    std::cout << "\n";
}