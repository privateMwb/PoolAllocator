// Bench Throughput
// Measures sustained allocation throughput under realistic allocation patterns.
//
// Covers:
// - fill-and-drain allocation cycles vs heap equivalent
// - interleaved allocation/deallocation vs heap equivalent

#include "bench_helper.h"

using namespace AllocatorPro;

static constexpr std::size_t kThroughputPoolSize = 256;

// Measures sustained throughput when repeatedly filling and draining the pool.
static void bench_fill_drain() {
    Pool pool{sizeof(Block), kThroughputPoolSize};
    void* blocks[kThroughputPoolSize]{};

    auto pool_fill_drain = [&] {
        (void)pool.allocateBatch(blocks);
        doNotOptimize();
        pool.deallocateBatch(blocks);
        doNotOptimize();
    };
    BENCH("pool_fill_drain", LARGE, pool_fill_drain);

    auto heap_fill_drain = [&] {
        for (std::size_t i = 0; i < kThroughputPoolSize; ++i)
            blocks[i] = ::operator new(sizeof(Block));
        doNotOptimize();
        for (std::size_t i = 0; i < kThroughputPoolSize; ++i)
            ::operator delete(blocks[i]);
        doNotOptimize();
    };
    BENCH("heap_fill_drain", LARGE, heap_fill_drain);
}

// Measures sustained throughput under alternating allocation and deallocation.
static void bench_interleaved() {
    Pool pool{sizeof(Block), kThroughputPoolSize};

    auto pool_interleaved = [&] {
        void* ptr = pool.allocate();
        doNotOptimize();
        pool.deallocate(ptr);
        doNotOptimize();
    };
    BENCH("pool_interleaved", LARGE, pool_interleaved);

    auto heap_interleaved = [&] {
        void* ptr = ::operator new(sizeof(Block));
        doNotOptimize();
        ::operator delete(ptr);
        doNotOptimize();
    };
    BENCH("heap_interleaved", LARGE, heap_interleaved);
}

// Executes all throughput benchmark cases.
void run_throughput_benchmarks() {
    setHeader("Throughput Benchmarks");

    bench_fill_drain();
    std::cout << "\n";

    bench_interleaved();
    borderLine();
    std::cout << "\n";
}