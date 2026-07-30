// Regression suite: AP_ASSERT(ptr != freeList_) in deallocate() catches
// an immediate double-free at zero release cost (see Pool.tpp). Because
// it's a plain assert(), an actual double-free would abort the process
// rather than something a test can safely trigger. This suite instead
// guards the other direction: patterns that *resemble* a double-free
// but are legitimate must never false-positive against that check.

#include <support/framework.h>

using namespace PoolPro;

// Verifies a full free -> reuse -> free cycle on the same address is not
// mistaken for a double-free, since the block is genuinely live again
// by the time it's freed the second time.
static void reuse_after_free_then_free_again_is_safe() {
    Pool<> pool(sizeof(void*), 4);
    void* a = pool.allocate();

    pool.deallocate(a);
    void* reused = pool.allocate();
    CHK(reused == a);

    pool.deallocate(reused); // legitimate: freeing what's currently live
    CHK(pool.usedBlocks() == 0);
    CHK(pool.freeBlocks() == 4);
}

// Verifies freeing two distinct live blocks back-to-back never trips the
// check, since the free-list head changes between the two calls.
static void freeing_two_different_blocks_consecutively_is_safe() {
    Pool<> pool(sizeof(void*), 4);
    void* a = pool.allocate();
    void* b = pool.allocate();

    pool.deallocate(b);
    pool.deallocate(a); // head is now `b`, not `a` — must not false-positive

    CHK(pool.freeBlocks() == 4);
    void* first = pool.allocate();
    CHK(first == a); // LIFO: `a` was freed last, so it comes back first
}

// Verifies a long run of consecutive frees of distinct blocks — the
// densest realistic case for a false positive — never trips the check.
static void many_sequential_frees_of_distinct_blocks_never_trip_assert() {
    constexpr std::size_t count = 8;
    Pool<> pool(sizeof(void*), count);

    void* blocks[count];
    for (std::size_t i = 0; i < count; ++i)
        blocks[i] = pool.allocate();

    for (std::size_t i = 0; i < count; ++i)
        pool.deallocate(blocks[i]); // each call sees a different current head

    CHK(pool.usedBlocks() == 0);
    CHK(pool.freeBlocks() == count);
}

// Executes all double-free-assert regression test cases.
static void run_tests() {
    RUN(reuse_after_free_then_free_again_is_safe);
    RUN(freeing_two_different_blocks_consecutively_is_safe);
    RUN(many_sequential_frees_of_distinct_blocks_never_trip_assert);
}

REGISTER_TEST_SUITE();
