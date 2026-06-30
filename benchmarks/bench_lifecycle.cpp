// Bench Object Lifecycle
// Measures the cost of object construction and destruction via
// pool create/destroy against heap new/delete.
//
// Covers:
// - Create vs heap new
// - Destroy vs heap delete
// - Create/destroy cycle vs heap new/delete cycle

#include "bench_helper.h"

using namespace AllocatorPro;

// Measures the cost of constructing an object in the pool versus heap allocation.
static void bench_create() {
    Pool pool{sizeof(Block), 16};

    auto pool_create = [&] {
        auto* b = pool.create<Block>();
        doNotOptimize();
        pool.destroy(b);
        doNotOptimize();
    };
    BENCH("pool_create", LARGE, pool_create);

    auto heap_create = [&] {
        auto* b = new Block{};
        doNotOptimize();
        delete b;
        doNotOptimize();
    };
    BENCH("heap_create", LARGE, heap_create);
}

// Measures the cost of destroying a pool-managed object versus heap deletion.
static void bench_destroy() {
    Pool pool{sizeof(Block), 16};

    auto pool_destroy = [&] {
        auto* b = pool.create<Block>();
        doNotOptimize();
        pool.destroy(b);
        doNotOptimize();
    };
    BENCH("pool_destroy", LARGE, pool_destroy);

    auto heap_destroy = [&] {
        auto* b = new Block{};
        doNotOptimize();
        delete b;
        doNotOptimize();
    };
    BENCH("heap_destroy", LARGE, heap_destroy);
}

// Measures the cost of repeated object creation and destruction versus heap new/delete.
static void bench_create_destroy_cycle() {
    Pool pool{sizeof(Block), 16};

    auto pool_cycle = [&] {
        auto* a = pool.create<Block>();
        auto* b = pool.create<Block>();
        doNotOptimize();
        pool.destroy(a);
        pool.destroy(b);
        doNotOptimize();
    };
    BENCH("pool_create_destroy_cycle", LARGE, pool_cycle);

    auto heap_cycle = [&] {
        auto* a = new Block{};
        auto* b = new Block{};
        doNotOptimize();
        delete a;
        delete b;
        doNotOptimize();
    };
    BENCH("heap_create_destroy_cycle", LARGE, heap_cycle);
}

// Executes all object lifecycle benchmark cases.
void run_lifecycle_benchmarks() {
    setHeader("Object Lifecycle Benchmarks");

    bench_create();
    std::cout << "\n";

    bench_destroy();
    std::cout << "\n";

    bench_create_destroy_cycle();
    borderLine();
    std::cout << "\n";
}