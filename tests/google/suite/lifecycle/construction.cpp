// Pool construction test suite.
//
// Coverage:
// - A newly constructed pool starts fully available (no blocks used)
// - totalBlocks() matches the requested block count
// - The pool is immediately usable — no lazy per-block setup is needed
//   before the first allocate()

#include <gtest/gtest.h>

#include <PoolPro/Pool.h>

#include <cstddef>

using namespace PoolPro;

// Verifies a fresh pool has every block free and none used.
TEST(Construction, StartsFullyAvailable) {
    Pool<> pool(sizeof(void*), 6);
    EXPECT_EQ(pool.usedBlocks(), 0u);
    EXPECT_EQ(pool.freeBlocks(), 6u);
}

// Verifies totalBlocks() reflects exactly what was requested.
TEST(Construction, BlockCountMatchesRequest) {
    Pool<> pool(sizeof(void*), 9);
    EXPECT_EQ(pool.totalBlocks(), 9u);
}

// Verifies every block is allocatable right after construction, with no
// warm-up pass required.
TEST(Construction, ImmediatelyUsableAfterConstruction) {
    Pool<> pool(sizeof(void*), 3);
    for (std::size_t i = 0; i < 3; ++i) {
        void* p = pool.allocate();
        EXPECT_NE(p, nullptr);
    }
    EXPECT_EQ(pool.allocate(), nullptr);
}
