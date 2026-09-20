// Pool allocate() test suite.
//
// Coverage:
// - Returns a non-null pointer within capacity
// - Returns nullptr once the pool is exhausted
// - Successive allocations advance usedBlocks()/freeBlocks()
// - A freed block is reused (LIFO) before virgin memory is touched

#include <gtest/gtest.h>

#include <PoolPro/Pool.h>

using namespace PoolPro;

// Verifies a request within capacity succeeds.
TEST(Allocate, ReturnsPointerWithinCapacity) {
    Pool<> pool(sizeof(void*), 4);
    void* p = pool.allocate();
    EXPECT_NE(p, nullptr);
}

// Verifies allocate() fails once every block has been handed out.
TEST(Allocate, ReturnsNullptrWhenExhausted) {
    Pool<> pool(sizeof(void*), 2);
    EXPECT_NE(pool.allocate(), nullptr);
    EXPECT_NE(pool.allocate(), nullptr);
    EXPECT_EQ(pool.allocate(), nullptr);
}

// Verifies usedBlocks()/freeBlocks() move in lockstep with each allocation.
TEST(Allocate, SuccessiveAllocationsAdvanceUsed) {
    Pool<> pool(sizeof(void*), 4);
    EXPECT_EQ(pool.usedBlocks(), 0u);

    (void)pool.allocate();
    EXPECT_EQ(pool.usedBlocks(), 1u);
    EXPECT_EQ(pool.freeBlocks(), 3u);

    (void)pool.allocate();
    EXPECT_EQ(pool.usedBlocks(), 2u);
    EXPECT_EQ(pool.freeBlocks(), 2u);
}

// Verifies a freed block is handed back out before the watermark
// advances into memory that has never been touched.
TEST(Allocate, ReusesFreedBlockBeforeWatermark) {
    Pool<> pool(sizeof(void*), 4);

    void* a = pool.allocate();
    void* b = pool.allocate();
    (void)b;
    pool.deallocate(a);

    // Two blocks are still virgin at this point (indices 2 and 3); if
    // acquireBlock() preferred the watermark over the free list, the
    // next allocate() would return a fresh block instead of `a`.
    void* reused = pool.allocate();
    EXPECT_EQ(reused, a);
}
