// Bench Constructor
// Measures the cost of pool construction and move operations
// against equivalent heap allocation and pointer operations.
//
// Covers:
// - Pool construction vs heap allocation
// - Move construction vs heap pointer transfer
// - Move assignment vs heap ownership transfer

#include "bench_helper.h"

using namespace AllocatorPro;

// Measures the cost of constructing a pool versus allocating an equivalent heap buffer.
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

// Measures the cost of move construction versus transferring heap pointer ownership.
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

// Measures the cost of move assignment versus transferring heap ownership.
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