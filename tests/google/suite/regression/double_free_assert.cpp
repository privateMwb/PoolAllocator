// Regression suite: AP_ASSERT(ptr != freeList_) in deallocate() catches
// an immediate double-free at zero release cost (see Pool.tpp). Because
// it's a plain assert(), an actual double-free would abort the process
// rather than something a test can safely trigger. This suite instead
// guards the other direction: patterns that *resemble* a double-free
// but are legitimate must never false-positive against that check.

#include <gtest/gtest.h>

#include <PoolPro/Pool.h>

#include <cstddef>

using namespace PoolPro;

// Verifies a full free -> reuse -> free cycle on the same address is not
// mistaken for a double-free, since the block is genuinely live again
// by the time it's freed the second time.
TEST(DoubleFreeAssert, ReuseAfterFreeThenFree) {
    Pool<> pool(sizeof(void*), 4);
    void* a = pool.allocate();

    pool.deallocate(a);
    void* reused = pool.allocate();
    EXPECT_EQ(reused, a);

    pool.deallocate(reused); // legitimate: freeing what's currently live
    EXPECT_EQ(pool.usedBlocks(), 0u);
    EXPECT_EQ(pool.freeBlocks(), 4u);
}

// Verifies freeing two distinct live blocks back-to-back never trips the
// check, since the free-list head changes between the two calls.
TEST(DoubleFreeAssert, FreeingTwoBlocksConsecutively) {
    Pool<> pool(sizeof(void*), 4);
    void* a = pool.allocate();
    void* b = pool.allocate();

    pool.deallocate(b);
    pool.deallocate(a); // head is now `b`, not `a` — must not false-positive

    EXPECT_EQ(pool.freeBlocks(), 4u);
    void* first = pool.allocate();
    EXPECT_EQ(first, a); // LIFO: `a` was freed last, so it comes back first
}

// Verifies a long run of consecutive frees of distinct blocks — the
// densest realistic case for a false positive — never trips the check.
TEST(DoubleFreeAssert, ManySequentialFreesNoAssert) {
    constexpr std::size_t count = 8;
    Pool<> pool(sizeof(void*), count);

    void* blocks[count];
    for (std::size_t i = 0; i < count; ++i)
        blocks[i] = pool.allocate();

    for (std::size_t i = 0; i < count; ++i)
        pool.deallocate(blocks[i]); // each call sees a different current head

    EXPECT_EQ(pool.usedBlocks(), 0u);
    EXPECT_EQ(pool.freeBlocks(), count);
}
