// Integration suite: allocate() + deallocate() + allocateBatch() working
// together, across states the unit-level allocateBatch() tests don't
// individually cover.
//
// Coverage:
// - A batch spanning a partially-populated free list AND remaining
//   virgin memory returns free-listed blocks first, then fresh ones,
//   with pool-wide counters ending up consistent
// - A batch drawn entirely from a free list once the watermark is
//   already fully advanced
// - A batch drawn entirely from virgin memory on a pool that has never
//   had anything freed

#include <gtest/gtest.h>

#include <PoolPro/Pool.h>

#include <cstddef>
#include <span>

using namespace PoolPro;

// Verifies a single allocateBatch() call correctly stitches together
// free-list reuse and watermark advancement when both are needed.
TEST(BatchFreelistDrain, BatchDrainsPartialAdvancesWatermark) {
    Pool<> pool(sizeof(void*), 8);

    void* blocks[4];
    for (int i = 0; i < 4; ++i)
        blocks[i] = pool.allocate();
    EXPECT_EQ(pool.usedBlocks(), 4u);
    EXPECT_EQ(pool.freeBlocks(), 4u); // 4 blocks still virgin

    // Free two of the four live blocks; free list is LIFO, so the most
    // recently freed one comes back out first.
    pool.deallocate(blocks[2]);
    pool.deallocate(blocks[0]);
    EXPECT_EQ(pool.freeBlocks(), 6u); // 2 free-listed + 4 virgin

    void* out[6];
    std::span<void*> span(out, 6);
    std::size_t got = pool.allocateBatch(span);

    EXPECT_EQ(got, 6u);
    EXPECT_EQ(out[0], blocks[0]);
    EXPECT_EQ(out[1], blocks[2]);
    for (int i = 2; i < 6; ++i) {
        for (int j = 2; j < i; ++j)
            EXPECT_NE(out[i], out[j]); // the virgin portion is all distinct
    }

    EXPECT_EQ(pool.usedBlocks(), 8u);
    EXPECT_EQ(pool.freeBlocks(), 0u);
}

// Verifies a batch is satisfied entirely from the free list once the
// watermark has nothing left to give.
TEST(BatchFreelistDrain, BatchDrainsFreelistWatermarkExhausted) {
    Pool<> pool(sizeof(void*), 4);
    void* blocks[4];
    for (int i = 0; i < 4; ++i)
        blocks[i] = pool.allocate();
    EXPECT_EQ(pool.freeBlocks(), 0u); // watermark fully advanced

    pool.deallocateBatch(std::span<void*>(blocks, 4));
    EXPECT_EQ(pool.freeBlocks(), 4u);

    void* out[4];
    std::span<void*> span(out, 4);
    std::size_t got = pool.allocateBatch(span);
    EXPECT_EQ(got, 4u);
    EXPECT_EQ(pool.freeBlocks(), 0u);
}

// Verifies a batch is satisfied entirely from virgin memory when nothing
// has ever been freed.
TEST(BatchFreelistDrain, BatchDrainsVirginMemoryEmptyFreelist) {
    Pool<> pool(sizeof(void*), 5);
    void* out[5];
    std::span<void*> span(out, 5);

    std::size_t got = pool.allocateBatch(span);
    EXPECT_EQ(got, 5u);
    EXPECT_EQ(pool.usedBlocks(), 5u);
}
