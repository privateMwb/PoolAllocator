// Integration suite: allocate() + reset() working together across full
// fill/drain cycles.
//
// Coverage:
// - Filling to capacity, resetting, and refilling reuses the exact same
//   addresses as the first pass (watermark truly rewinds, not just the
//   used-block counter)
// - reset() after a partial fill discards free-list state, not just
//   unused watermark capacity
// - The pool survives many repeated fill/reset cycles without drift

#include <gtest/gtest.h>

#include <PoolPro/Pool.h>

using namespace PoolPro;

// Verifies a second fill after reset() lines up address-for-address with
// the first fill.
TEST(AllocResetReuse, FullCycleAfterResetMatches) {
    Pool<> pool(sizeof(void*), 4);

    void* first[4];
    for (int i = 0; i < 4; ++i) {
        first[i] = pool.allocate();
        EXPECT_NE(first[i], nullptr);
    }
    EXPECT_EQ(pool.allocate(), nullptr);

    pool.reset();
    EXPECT_EQ(pool.usedBlocks(), 0u);
    EXPECT_EQ(pool.freeBlocks(), 4u);

    void* second[4];
    for (int i = 0; i < 4; ++i) {
        second[i] = pool.allocate();
        EXPECT_NE(second[i], nullptr);
    }
    EXPECT_EQ(pool.allocate(), nullptr);

    for (int i = 0; i < 4; ++i)
        EXPECT_EQ(second[i], first[i]);
}

// Verifies reset() clears free-listed blocks too, not just untouched
// watermark capacity.
TEST(AllocResetReuse, ResetPartialUseDiscardsFreelist) {
    Pool<> pool(sizeof(void*), 4);
    void* a = pool.allocate();
    (void)pool.allocate();
    pool.deallocate(a);
    EXPECT_EQ(pool.freeBlocks(), 3u); // 1 free-listed + 2 virgin

    pool.reset();
    EXPECT_EQ(pool.freeBlocks(), 4u);
    EXPECT_EQ(pool.usedBlocks(), 0u);

    void* reused = pool.allocate();
    EXPECT_EQ(reused, a); // watermark rewound to block 0, same address as `a`
}

// Verifies repeated fill/reset cycles remain stable rather than drifting
// or leaking capacity over time.
TEST(AllocResetReuse, SurvivesMultipleResetCycles) {
    Pool<> pool(sizeof(void*), 3);
    for (int cycle = 0; cycle < 5; ++cycle) {
        for (int i = 0; i < 3; ++i)
            EXPECT_NE(pool.allocate(), nullptr);
        EXPECT_EQ(pool.allocate(), nullptr);

        pool.reset();
        EXPECT_EQ(pool.usedBlocks(), 0u);
        EXPECT_EQ(pool.freeBlocks(), 3u);
    }
}
