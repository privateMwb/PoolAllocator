// Pool allocate() test suite.
//
// Coverage:
// - Returns a non-null pointer within capacity
// - Returns nullptr once the pool is exhausted
// - Successive allocations advance usedBlocks()/freeBlocks()
// - A freed block is reused (LIFO) before virgin memory is touched

#include <support/framework.h>

using namespace PoolPro;

// Verifies a request within capacity succeeds.
static void returns_pointer_within_capacity() {
    Pool<> pool(sizeof(void*), 4);
    void* p = pool.allocate();
    CHK(p != nullptr);
}

// Verifies allocate() fails once every block has been handed out.
static void returns_nullptr_when_exhausted() {
    Pool<> pool(sizeof(void*), 2);
    CHK(pool.allocate() != nullptr);
    CHK(pool.allocate() != nullptr);
    CHK(pool.allocate() == nullptr);
}

// Verifies usedBlocks()/freeBlocks() move in lockstep with each allocation.
static void successive_allocations_advance_used() {
    Pool<> pool(sizeof(void*), 4);
    CHK(pool.usedBlocks() == 0);

    (void)pool.allocate();
    CHK(pool.usedBlocks() == 1);
    CHK(pool.freeBlocks() == 3);

    (void)pool.allocate();
    CHK(pool.usedBlocks() == 2);
    CHK(pool.freeBlocks() == 2);
}

// Verifies a freed block is handed back out before the watermark
// advances into memory that has never been touched.
static void reuses_freed_block_before_watermark() {
    Pool<> pool(sizeof(void*), 4);

    void* a = pool.allocate();
    void* b = pool.allocate();
    (void)b;
    pool.deallocate(a);

    // Two blocks are still virgin at this point (indices 2 and 3); if
    // acquireBlock() preferred the watermark over the free list, the
    // next allocate() would return a fresh block instead of `a`.
    void* reused = pool.allocate();
    CHK(reused == a);
}

// Executes all allocate() test cases.
static void run_tests() {
    RUN(returns_pointer_within_capacity);
    RUN(returns_nullptr_when_exhausted);
    RUN(successive_allocations_advance_used);
    RUN(reuses_freed_block_before_watermark);
}

REGISTER_TEST_SUITE();
