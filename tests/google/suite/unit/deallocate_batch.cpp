// Pool deallocateBatch() test suite.
//
// Coverage:
// - All owned pointers in the span are returned to the pool
// - nullptr and foreign pointers within the span are skipped
// - Blocks freed as a batch are reusable afterward
// - freeBlocks() reflects only the pointers actually owned by the pool

#include <gtest/gtest.h>

#include <PoolPro/Pool.h>

#include <cstddef>
#include <span>

using namespace PoolPro;

// Verifies a batch of entirely owned pointers is fully released.
TEST(DeallocateBatch, ReturnsAllOwnedPointers) {
    Pool<> pool(sizeof(void*), 4);
    void* out[4];
    std::span<void*> outSpan(out, 4);
    (void)pool.allocateBatch(outSpan);
    EXPECT_EQ(pool.usedBlocks(), 4u);

    pool.deallocateBatch(outSpan);
    EXPECT_EQ(pool.usedBlocks(), 0u);
    EXPECT_EQ(pool.freeBlocks(), 4u);
}

// Verifies nullptr and foreign pointers mixed into the span are skipped
// rather than corrupting the free list.
TEST(DeallocateBatch, SkipsNullAndForeignPointers) {
    Pool<> pool(sizeof(void*), 4);
    void* a = pool.allocate();
    int stackVar = 0;
    void* mixed[3] = {a, nullptr, &stackVar};
    std::span<void*> mixedSpan(mixed, 3);

    pool.deallocateBatch(mixedSpan);
    EXPECT_EQ(pool.usedBlocks(), 0u);
    EXPECT_EQ(pool.freeBlocks(), 4u);
}

// Verifies blocks released via deallocateBatch() are handed back out.
TEST(DeallocateBatch, FreedBlocksAreReusable) {
    Pool<> pool(sizeof(void*), 2);
    void* out[2];
    std::span<void*> outSpan(out, 2);
    (void)pool.allocateBatch(outSpan);

    pool.deallocateBatch(outSpan);

    void* in[2];
    std::span<void*> inSpan(in, 2);
    std::size_t got = pool.allocateBatch(inSpan);
    EXPECT_EQ(got, 2u);
}

// Verifies freeBlocks() only counts pointers that were actually owned.
TEST(DeallocateBatch, FreeBlocksReflectsOnlyValidPointers) {
    Pool<> pool(sizeof(void*), 4);
    void* a = pool.allocate();
    (void)pool.allocate();
    void* mixed[2] = {a, nullptr};
    std::span<void*> mixedSpan(mixed, 2);

    pool.deallocateBatch(mixedSpan);
    EXPECT_EQ(pool.freeBlocks(), 3u); // 2 still-virgin blocks + the one valid pointer freed
}
