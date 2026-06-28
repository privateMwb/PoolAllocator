// Pool Management Test Suite
//
// Coverage:
// - Pool reset
// - Reset preserves object lifetime
// - Statistics reset
// - Allocation after reset

#include "test_helper.h"

using namespace AllocatorPro;

// Reset Restores Free List
// Verifies that reset() returns all blocks to the pool.
static void reset_restores_free_list() {
    Pool pool{sizeof(Tracker), 4};

    (void)pool.allocate();
    (void)pool.allocate();
    pool.reset();

    CHK(pool.freeBlocks() == 4);
    CHK(pool.usedBlocks() == 0);
}

// Reset Preserves Object Lifetime
// Verifies that reset() does not invoke object destructors.
static void reset_does_not_call_destructors() {
    Pool pool{sizeof(Tracker), 4};
    Tracker::reset();

    (void)pool.create<Tracker>();
    (void)pool.create<Tracker>();
    pool.reset();

    CHK(Tracker::destructions == 0);
}

// Reset Clears Statistics
// Verifies that reset() restores all allocator statistics to their initial state.
static void reset_clears_stats() {
    Pool<true> pool{sizeof(Tracker), 4};

    (void)pool.allocate();
    (void)pool.allocate();
    pool.reset();

    CHK(pool.getStats().allocations_    == 0);
    CHK(pool.getStats().deallocations_  == 0);
    CHK(pool.getStats().totalAllocated_ == 0);
    CHK(pool.getStats().peakUsed_       == 0);
}

// Allocate After Reset
// Verifies that allocations succeed normally after a pool reset.
static void allocate_after_reset() {
    Pool pool{sizeof(Tracker), 4};
    Tracker::reset();

    (void)pool.allocate();
    (void)pool.allocate();
    pool.reset();

    Tracker* t = pool.create<Tracker>(99);

    CHK(t != nullptr);
    CHK(t->value == 99);
    CHK(pool.usedBlocks() == 1);
    CHK(pool.freeBlocks() == 3);

    pool.destroy(t);
}

// Test Runner
// Executes all pool management test cases.
void run_pool_management_tests() {
    setTitle("Pool Management");
    
    RUN(reset_restores_free_list);
    RUN(reset_does_not_call_destructors);
    RUN(reset_clears_stats);
    RUN(allocate_after_reset);

    std::cout << "\n";
}