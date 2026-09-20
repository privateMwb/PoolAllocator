// Pool block-introspection test suite: capacity(), usedBlocks(),
// freeBlocks(), totalBlocks(), blockStride().
//
// Coverage:
// - totalBlocks() matches the count given at construction
// - blockStride() is at least the requested block size and alignment-rounded
// - capacity() equals blockStride() * totalBlocks()
// - usedBlocks() + freeBlocks() always equals totalBlocks()

#include <gtest/gtest.h>

#include <PoolPro/Pool.h>

using namespace PoolPro;

// Verifies totalBlocks() reflects the constructor argument.
TEST(BlockIntrospection, TotalBlocksMatchesConstruction) {
    Pool<> pool(sizeof(void*), 7);
    EXPECT_EQ(pool.totalBlocks(), 7u);
}

// Verifies blockStride() is large enough to hold a block and satisfies
// the requested alignment.
TEST(BlockIntrospection, BlockStrideAtLeastBlockSize) {
    Pool<> pool(24, 4, 16);
    EXPECT_GE(pool.blockStride(), 24u);
    EXPECT_EQ(pool.blockStride() % 16, 0u);
}

// Verifies capacity() is derived from stride and block count, not tracked
// independently.
TEST(BlockIntrospection, CapacityEqualsStrideTimesTotal) {
    Pool<> pool(sizeof(void*), 5);
    EXPECT_EQ(pool.capacity(), pool.blockStride() * pool.totalBlocks());
}

// Verifies used and free block counts always sum to the total, through
// a mix of allocation and deallocation.
TEST(BlockIntrospection, UsedAndFreeSumToTotal) {
    Pool<> pool(sizeof(void*), 4);
    void* a = pool.allocate();
    (void)pool.allocate();
    EXPECT_EQ(pool.usedBlocks() + pool.freeBlocks(), pool.totalBlocks());

    pool.deallocate(a);
    EXPECT_EQ(pool.usedBlocks() + pool.freeBlocks(), pool.totalBlocks());
}
