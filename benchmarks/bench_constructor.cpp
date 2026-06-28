// Bench Constructor 
// Measures the cost of pool construction and destruction
// against equivalent heap allocation and deallocation.
//
// Covers:
// - pool construction vs heap allocation of equivalent size
// - pool destruction vs heap deallocation
// - move construction vs heap pointer reassignment
// - move assignment vs heap reallocation and pointer swap

#include "bench_helper.h"

using namespace AllocatorPro;

// Construct
// measures pool construction vs equivalent heap allocation and deallocation
static void bench_construct() {
    auto pool_construct = [] {
        Pool pool{sizeof(Block), 16};
        doNotOptimize();
    };
    BENCH("pool_construct", LARGE, pool_construct);

    auto heap_construct = [] {
        void* p = ::operator new(sizeof(Block) * 16);
        doNotOptimize();
        ::operator delete(p);
        doNotOptimize();
    };
    BENCH("heap_construct", LARGE, heap_construct);
}

// Move Construct
// measures pool move construction vs heap pointer reassignment
static void bench_move_construct() {
    auto pool_move_construct = [] {
        Pool a{sizeof(Block), 16};
        Pool b{std::move(a)};
        doNotOptimize();
    };
    BENCH("pool_move_construct", LARGE, pool_move_construct);

    auto heap_move_construct = [] {
        void* a = ::operator new(sizeof(Block) * 16);
        void* b = a;
        doNotOptimize();
        ::operator delete(a);
        doNotOptimize();
    };
    BENCH("heap_move_construct", LARGE, heap_move_construct);
}

// Move Assign
// measures pool move assignment vs heap reallocation and pointer swap
static void bench_move_assign() {
    auto pool_move_assign = [] {
        Pool a{sizeof(Block), 16};
        Pool b{sizeof(Block), 8};
        b = std::move(a);
        doNotOptimize();
    };
    BENCH("pool_move_assign", LARGE, pool_move_assign);

    auto heap_move_assign = [] {
        void* a = ::operator new(sizeof(Block) * 16);
        void* b = ::operator new(sizeof(Block) * 8);
        ::operator delete(b);
        b = a;
        doNotOptimize();
        ::operator delete(a);
        doNotOptimize();
    };
    BENCH("heap_move_assign", LARGE, heap_move_assign);
}

// Benchmark Runner
// Executes all constructor benchmark cases.
void run_constructor_benchmarks() {
    setHeader("Constructor Benchmarks");
    
    bench_construct();
    std::cout << "\n";

    bench_move_construct();
    std::cout << "\n";

    bench_move_assign();
    borderLine();
    std::cout << "\n";
}