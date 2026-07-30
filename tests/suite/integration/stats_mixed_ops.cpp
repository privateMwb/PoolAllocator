// Integration suite: stats tracking across a long, mixed sequence of
// allocate()/deallocate()/allocateBatch()/deallocateBatch()/reset()
// calls, as opposed to the single-call focus of the unit-level stats
// suite.
//
// Coverage:
// - Running totals stay correct through an interleaved workflow mixing
//   single and batch operations
// - The peak-used high-water mark never drops except via reset()
// - reset() mid-workflow restarts peak tracking from zero for whatever
//   happens next

#include <support/framework.h>

using namespace PoolPro;

// Verifies stats stay internally consistent across a realistic mixed
// sequence of single and batch calls.
static void stats_stay_consistent_across_mixed_workflow() {
    Pool<true> pool(sizeof(void*), 6);

    void* a = pool.allocate();
    void* b = pool.allocate();
    CHK(pool.getStats().totalAllocated_ == 2);
    CHK(pool.getStats().peakUsed_ == 2);

    pool.deallocate(a);
    CHK(pool.getStats().deallocations_ == 1);
    CHK(pool.getStats().peakUsed_ == 2); // freeing never lowers the peak

    void* batchOut[4];
    std::span<void*> span(batchOut, 4);
    std::size_t got = pool.allocateBatch(span);
    CHK(got == 4);
    CHK(pool.getStats().totalAllocated_ == 6); // 2 + 4
    CHK(pool.getStats().peakUsed_ == 5);       // b still live + 4 fresh = 5 of 6
    CHK(pool.usedBlocks() == 5);

    pool.deallocateBatch(std::span<void*>(batchOut, 4));
    CHK(pool.getStats().deallocations_ == 5); // 1 + 4
    CHK(pool.getStats().peakUsed_ == 5);      // still the high-water mark

    pool.deallocate(b);
    CHK(pool.usedBlocks() == 0);
    CHK(pool.getStats().peakUsed_ == 5);
}

// Verifies reset() partway through a workflow restarts peak tracking,
// rather than the new peak being measured against the old one.
static void reset_midway_restarts_peak_tracking() {
    Pool<true> pool(sizeof(void*), 4);

    (void)pool.allocate();
    (void)pool.allocate();
    (void)pool.allocate();
    CHK(pool.getStats().peakUsed_ == 3);

    pool.reset();
    CHK(pool.getStats().peakUsed_ == 0);

    (void)pool.allocate();
    CHK(pool.getStats().peakUsed_ == 1); // does not remember the earlier peak of 3
}

// Executes all mixed-workflow stats test cases.
static void run_tests() {
    RUN(stats_stay_consistent_across_mixed_workflow);
    RUN(reset_midway_restarts_peak_tracking);
}

REGISTER_TEST_SUITE();
