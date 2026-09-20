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

#include <gtest/gtest.h>

#include <PoolPro/Pool.h>

#include <cstddef>
#include <span>

using namespace PoolPro;

// Verifies stats stay internally consistent across a realistic mixed
// sequence of single and batch calls.
TEST(StatsMixedOps, StatsConsistentMixedWorkflow) {
    Pool<true> pool(sizeof(void*), 6);

    void* a = pool.allocate();
    void* b = pool.allocate();
    EXPECT_EQ(pool.getStats().totalAllocated_, 2u);
    EXPECT_EQ(pool.getStats().peakUsed_, 2u);

    pool.deallocate(a);
    EXPECT_EQ(pool.getStats().deallocations_, 1u);
    EXPECT_EQ(pool.getStats().peakUsed_, 2u); // freeing never lowers the peak

    void* batchOut[4];
    std::span<void*> span(batchOut, 4);
    std::size_t got = pool.allocateBatch(span);
    EXPECT_EQ(got, 4u);
    EXPECT_EQ(pool.getStats().totalAllocated_, 6u); // 2 + 4
    EXPECT_EQ(pool.getStats().peakUsed_, 5u);       // b still live + 4 fresh = 5 of 6
    EXPECT_EQ(pool.usedBlocks(), 5u);

    pool.deallocateBatch(std::span<void*>(batchOut, 4));
    EXPECT_EQ(pool.getStats().deallocations_, 5u); // 1 + 4
    EXPECT_EQ(pool.getStats().peakUsed_, 5u);      // still the high-water mark

    pool.deallocate(b);
    EXPECT_EQ(pool.usedBlocks(), 0u);
    EXPECT_EQ(pool.getStats().peakUsed_, 5u);
}

// Verifies reset() partway through a workflow restarts peak tracking,
// rather than the new peak being measured against the old one.
TEST(StatsMixedOps, ResetMidwayRestartsPeak) {
    Pool<true> pool(sizeof(void*), 4);

    (void)pool.allocate();
    (void)pool.allocate();
    (void)pool.allocate();
    EXPECT_EQ(pool.getStats().peakUsed_, 3u);

    pool.reset();
    EXPECT_EQ(pool.getStats().peakUsed_, 0u);

    (void)pool.allocate();
    EXPECT_EQ(pool.getStats().peakUsed_, 1u); // does not remember the earlier peak of 3
}
