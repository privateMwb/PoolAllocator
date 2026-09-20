// Pool reset() test suite.
//
// Coverage:
// - Restores full availability (clears free list and watermark)
// - capacity()/totalBlocks() are unchanged by reset()
// - Allocation resumes from block 0 after a reset
// - Stats are cleared when EnableStats is set

#include <gtest/gtest.h>

#include <PoolPro/Pool.h>

#include <cstddef>

using namespace PoolPro;

// Verifies reset() clears both used blocks and the free list, making
// every block available again.
TEST(Reset, RestoresFullAvailability) {
    Pool<> pool(sizeof(void*), 4);
    (void)pool.allocate();
    (void)pool.allocate();
    EXPECT_EQ(pool.usedBlocks(), 2u);

    pool.reset();
    EXPECT_EQ(pool.usedBlocks(), 0u);
    EXPECT_EQ(pool.freeBlocks(), 4u);
}

// Verifies reset() does not shrink or resize the pool itself.
TEST(Reset, CapacityUnchangedAfterReset) {
    Pool<> pool(sizeof(void*), 4);
    const std::size_t before = pool.capacity();

    (void)pool.allocate();
    pool.reset();

    EXPECT_EQ(pool.capacity(), before);
    EXPECT_EQ(pool.totalBlocks(), 4u);
}

// Verifies allocation resumes from the first block after a reset,
// confirming the watermark itself was rewound, not just the free list.
TEST(Reset, ResetReusesFromStart) {
    Pool<> pool(sizeof(void*), 4);
    void* first = pool.allocate();
    (void)pool.allocate();

    pool.reset();
    void* afterReset = pool.allocate();
    EXPECT_EQ(afterReset, first);
}

// Verifies reset() zeroes out accumulated stats when stats are enabled.
TEST(Reset, ClearsStatsWhenEnabled) {
    Pool<true> pool(sizeof(void*), 4);
    (void)pool.allocate();
    (void)pool.allocate();
    EXPECT_EQ(pool.getStats().totalAllocated_, 2u);

    pool.reset();
    const auto& stats = pool.getStats();
    EXPECT_EQ(stats.totalAllocated_, 0u);
    EXPECT_EQ(stats.peakUsed_, 0u);
    EXPECT_EQ(stats.allocations_, 0u);
    EXPECT_EQ(stats.deallocations_, 0u);
}
