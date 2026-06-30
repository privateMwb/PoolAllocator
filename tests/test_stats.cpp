// Pool statistics test suite.
//
// Coverage:
// - Initial statistics are zero
// - Allocation increments allocation count
// - Allocation updates total allocated
// - Deallocation increments deallocation count
// - Allocation updates peak used
// - Peak used does not decrease on free
// - Peak used does not decrease on reset
// - Statistics reset on reset
// - create/destroy update statistics
// - allocateBatch updates statistics per block

#include "test_helper.h"

using namespace AllocatorPro;

// Verifies that all statistics are initialized to zero.
static void initial_stats_are_zero() {
    Pool<true> pool{64, 4};
    const auto& stats = pool.getStats();
    CHK(stats.totalAllocated_ == 0);
    CHK(stats.peakUsed_       == 0);
    CHK(stats.allocations_    == 0);
    CHK(stats.deallocations_  == 0);
}

// Verifies that each allocation increments the allocation count.
static void allocation_increments_count() {
    Pool<true> pool{64, 4};
    (void)pool.allocate();
    (void)pool.allocate();
    CHK(pool.getStats().allocations_ == 2);
}

// Verifies that total allocated count accumulates across allocations.
static void allocation_updates_total_allocated() {
    Pool<true> pool{64, 4};
    (void)pool.allocate();
    (void)pool.allocate();
    (void)pool.allocate();
    CHK(pool.getStats().totalAllocated_ == 3);
}

// Verifies that each deallocation increments the deallocation count.
static void deallocation_increments_count() {
    Pool<true> pool{64, 4};
    void* a = pool.allocate();
    void* b = pool.allocate();
    pool.deallocate(a);
    pool.deallocate(b);
    CHK(pool.getStats().deallocations_ == 2);
}

// Verifies that peak used blocks track the highest usage level reached.
static void allocation_updates_peak_used() {
    Pool<true> pool{64, 4};
    (void)pool.allocate();
    (void)pool.allocate();
    (void)pool.allocate();
    CHK(pool.getStats().peakUsed_ == 3);
}

// Verifies that peak used blocks are preserved after freeing memory.
static void peak_used_does_not_decrease_on_free() {
    Pool<true> pool{64, 4};
    void* a = pool.allocate();
    void* b = pool.allocate();
    pool.deallocate(a);
    pool.deallocate(b);
    CHK(pool.getStats().peakUsed_ == 2);
}

// Verifies that peak used blocks are not preserved after reset.
static void peak_used_on_reset() {
    Pool<true> pool{64, 4};
    (void)pool.allocate();
    (void)pool.allocate();
    pool.reset();
    CHK(pool.getStats().peakUsed_ == 0);
}

// Verifies that reset clears the runtime statistics counters.
static void stats_reset_on_reset() {
    Pool<true> pool{64, 4};
    (void)pool.allocate();
    pool.reset();
    const auto& stats = pool.getStats();
    CHK(stats.totalAllocated_ == 0);
    CHK(stats.allocations_    == 0);
    CHK(stats.deallocations_  == 0);
}

// Verifies that create and destroy update statistics the same as
// raw allocate and deallocate.
static void create_destroy_update_stats() {
    Pool<true> pool{sizeof(Tracker), 4};
    Tracker::reset();

    Tracker* t = pool.create<Tracker>(1);
    pool.destroy(t);

    CHK(pool.getStats().allocations_   == 1);
    CHK(pool.getStats().deallocations_ == 1);
}

// Verifies that batch allocation updates statistics per block allocated.
static void batch_allocate_updates_stats() {
    Pool<true> pool{64, 4};
    void* blocks[4]{};
    (void)pool.allocateBatch(blocks);

    CHK(pool.getStats().allocations_    == 4);
    CHK(pool.getStats().totalAllocated_ == 4);
    CHK(pool.getStats().peakUsed_       == 4);
}

// Executes all statistics test cases.
void run_stats_tests() {
    setTitle("Statistics");

    RUN(initial_stats_are_zero);
    RUN(allocation_increments_count);
    RUN(allocation_updates_total_allocated);
    RUN(deallocation_increments_count);
    RUN(allocation_updates_peak_used);
    RUN(peak_used_does_not_decrease_on_free);
    RUN(peak_used_on_reset);
    RUN(stats_reset_on_reset);
    RUN(create_destroy_update_stats);
    RUN(batch_allocate_updates_stats);

    std::cout << "\n";
}
