// Pool deallocateBatch() test suite.
//
// Coverage:
// - All owned pointers in the span are returned to the pool
// - nullptr and foreign pointers within the span are skipped
// - Blocks freed as a batch are reusable afterward
// - freeBlocks() reflects only the pointers actually owned by the pool

#include <support/framework.h>

using namespace PoolPro;

// Verifies a batch of entirely owned pointers is fully released.
static void returns_all_owned_pointers() {
    Pool<> pool(sizeof(void*), 4);
    void* out[4];
    std::span<void*> outSpan(out, 4);
    (void)pool.allocateBatch(outSpan);
    CHK(pool.usedBlocks() == 4);

    pool.deallocateBatch(outSpan);
    CHK(pool.usedBlocks() == 0);
    CHK(pool.freeBlocks() == 4);
}

// Verifies nullptr and foreign pointers mixed into the span are skipped
// rather than corrupting the free list.
static void skips_null_and_foreign_pointers() {
    Pool<> pool(sizeof(void*), 4);
    void* a = pool.allocate();
    int stackVar = 0;
    void* mixed[3] = {a, nullptr, &stackVar};
    std::span<void*> mixedSpan(mixed, 3);

    pool.deallocateBatch(mixedSpan);
    CHK(pool.usedBlocks() == 0);
    CHK(pool.freeBlocks() == 4);
}

// Verifies blocks released via deallocateBatch() are handed back out.
static void freed_blocks_are_reusable() {
    Pool<> pool(sizeof(void*), 2);
    void* out[2];
    std::span<void*> outSpan(out, 2);
    (void)pool.allocateBatch(outSpan);

    pool.deallocateBatch(outSpan);

    void* in[2];
    std::span<void*> inSpan(in, 2);
    std::size_t got = pool.allocateBatch(inSpan);
    CHK(got == 2);
}

// Verifies freeBlocks() only counts pointers that were actually owned.
static void free_blocks_reflects_only_valid_pointers() {
    Pool<> pool(sizeof(void*), 4);
    void* a = pool.allocate();
    (void)pool.allocate();
    void* mixed[2] = {a, nullptr};
    std::span<void*> mixedSpan(mixed, 2);

    pool.deallocateBatch(mixedSpan);
    CHK(pool.freeBlocks() == 3); // 2 still-virgin blocks + the one valid pointer freed
}

// Executes all deallocateBatch() test cases.
static void run_tests() {
    RUN(returns_all_owned_pointers);
    RUN(skips_null_and_foreign_pointers);
    RUN(freed_blocks_are_reusable);
    RUN(free_blocks_reflects_only_valid_pointers);
}

REGISTER_TEST_SUITE();
