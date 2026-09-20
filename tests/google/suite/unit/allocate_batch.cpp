// Pool allocateBatch() test suite.
//
// Coverage:
// - Fills the whole span when capacity allows
// - Returns a partial count once the pool runs out mid-batch
// - Drains the free list before advancing into virgin memory
// - freeBlocks() reflects the whole batch in one update

#include <gtest/gtest.h>

#include <PoolPro/Pool.h>

#include <cstddef>
#include <span>

using namespace PoolPro;

// Verifies a batch that fits entirely within capacity is filled completely.
TEST(AllocateBatch, FillsAllWhenCapacityAllows) {
    Pool<> pool(sizeof(void*), 4);
    void* out[4];
    std::span<void*> span(out, 4);

    std::size_t got = pool.allocateBatch(span);
    EXPECT_EQ(got, 4u);
    EXPECT_EQ(pool.usedBlocks(), 4u);
}

// Verifies a batch larger than remaining capacity returns fewer blocks
// than requested, rather than failing outright.
TEST(AllocateBatch, ReturnsPartialCountWhenExhausted) {
    Pool<> pool(sizeof(void*), 2);
    void* out[5];
    std::span<void*> span(out, 5);

    std::size_t got = pool.allocateBatch(span);
    EXPECT_EQ(got, 2u);
}

// Verifies free-listed blocks are handed out (LIFO) before the watermark
// advances into blocks that have never been touched.
TEST(AllocateBatch, DrainsFreeListBeforeWatermark) {
    Pool<> pool(sizeof(void*), 4);
    void* a = pool.allocate();
    void* b = pool.allocate();
    pool.deallocate(a);
    pool.deallocate(b); // free list is now: b -> a

    void* out[4];
    std::span<void*> span(out, 4);
    std::size_t got = pool.allocateBatch(span);

    EXPECT_EQ(got, 4u);
    EXPECT_EQ(out[0], b);
    EXPECT_EQ(out[1], a);
}

// Verifies freeBlocks() is updated once for the whole batch.
TEST(AllocateBatch, FreeBlocksUpdatesAfterBatch) {
    Pool<> pool(sizeof(void*), 4);
    void* out[3];
    std::span<void*> span(out, 3);

    (void)pool.allocateBatch(span);
    EXPECT_EQ(pool.freeBlocks(), 1u);
}
