// Pool deallocate() test suite.
//
// Coverage:
// - nullptr is silently ignored
// - A pointer not owned by the pool is silently ignored
// - An owned pointer is returned to the pool (usedBlocks()/freeBlocks() update)
// - A freed block is handed back out by the next allocate() (LIFO reuse)

#include <gtest/gtest.h>

#include <PoolPro/Pool.h>

using namespace PoolPro;

// Verifies deallocate(nullptr) is a no-op.
TEST(Deallocate, IgnoresNullPointer) {
    Pool<> pool(sizeof(void*), 4);
    pool.deallocate(nullptr);
    EXPECT_EQ(pool.usedBlocks(), 0u);
}

// Verifies a foreign pointer is rejected without corrupting pool state.
TEST(Deallocate, IgnoresForeignPointer) {
    Pool<> pool(sizeof(void*), 4);
    int stackVar = 0;
    pool.deallocate(&stackVar);
    EXPECT_EQ(pool.freeBlocks(), 4u);
}

// Verifies an owned pointer is released back to the pool.
TEST(Deallocate, ReturnsBlockToPool) {
    Pool<> pool(sizeof(void*), 4);
    void* p = pool.allocate();
    EXPECT_EQ(pool.usedBlocks(), 1u);

    pool.deallocate(p);
    EXPECT_EQ(pool.usedBlocks(), 0u);
    EXPECT_EQ(pool.freeBlocks(), 4u);
}

// Verifies a freed block is the next one handed out.
TEST(Deallocate, FreedBlockIsReusable) {
    Pool<> pool(sizeof(void*), 1);
    void* p = pool.allocate();
    pool.deallocate(p);

    void* reused = pool.allocate();
    EXPECT_EQ(reused, p);
}
