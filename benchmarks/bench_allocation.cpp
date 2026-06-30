// Bench Allocation
// Measures the cost of pool allocation and deallocation
// against equivalent heap allocation and deallocation.
//
// Covers:
// - Single allocate/deallocate vs heap new/delete
// - Full allocation/deallocation cycle vs heap new/delete cycle

#include "bench_helper.h"

using namespace AllocatorPro;

// Measures the cost of a single pool allocation and deallocation versus heap new/delete.
static void bench_allocate() {
    Pool pool{sizeof(Block), 16};

    auto pool_allocate = [&] {
        void* ptr = pool.allocate();
        doNotOptimize();
        pool.deallocate(ptr);
        doNotOptimize();
    };
    BENCH("pool_allocate", LARGE, pool_allocate);

    auto heap_allocate = [&] {
        void* ptr = ::operator new(sizeof(Block));
        doNotOptimize();
        ::operator delete(ptr);
        doNotOptimize();
    };
    BENCH("heap_allocate", LARGE, heap_allocate);
}

// Measures the cost of repeated allocation and deallocation cycles versus heap new/delete.
static void bench_alloc_dealloc_cycle() {
    Pool pool{sizeof(Block), 16};

    auto pool_cycle = [&] {
        void* a = pool.allocate();
        void* b = pool.allocate();
        doNotOptimize();
        pool.deallocate(a);
        pool.deallocate(b);
        doNotOptimize();
    };
    BENCH("pool_alloc_dealloc_cycle", LARGE, pool_cycle);

    auto heap_cycle = [&] {
        void* a = ::operator new(sizeof(Block));
        void* b = ::operator new(sizeof(Block));
        doNotOptimize();
        ::operator delete(a);
        ::operator delete(b);
        doNotOptimize();
    };
    BENCH("heap_alloc_dealloc_cycle", LARGE, heap_cycle);
}

// Executes all allocation benchmark cases.
void run_allocation_benchmarks() {
    setHeader("Allocation Benchmarks");

    bench_allocate();
    std::cout << "\n";

    bench_alloc_dealloc_cycle();
    borderLine();
    std::cout << "\n";
}