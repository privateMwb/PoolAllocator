// Pool allocateBatch() test suite.
//
// Coverage:
// - Fills the whole span when capacity allows
// - Returns a partial count once the pool runs out mid-batch
// - Drains the free list before advancing into virgin memory
// - freeBlocks() reflects the whole batch in one update

#include <support/framework.h>

using namespace PoolPro;

// Verifies a batch that fits entirely within capacity is filled completely.
static void fills_all_when_capacity_allows() {
    Pool<> pool(sizeof(void*), 4);
    void* out[4];
    std::span<void*> span(out, 4);

    std::size_t got = pool.allocateBatch(span);
    CHK(got == 4);
    CHK(pool.usedBlocks() == 4);
}

// Verifies a batch larger than remaining capacity returns fewer blocks
// than requested, rather than failing outright.
static void returns_partial_count_when_exhausted() {
    Pool<> pool(sizeof(void*), 2);
    void* out[5];
    std::span<void*> span(out, 5);

    std::size_t got = pool.allocateBatch(span);
    CHK(got == 2);
}

// Verifies free-listed blocks are handed out (LIFO) before the watermark
// advances into blocks that have never been touched.
static void drains_free_list_before_watermark() {
    Pool<> pool(sizeof(void*), 4);
    void* a = pool.allocate();
    void* b = pool.allocate();
    pool.deallocate(a);
    pool.deallocate(b); // free list is now: b -> a

    void* out[4];
    std::span<void*> span(out, 4);
    std::size_t got = pool.allocateBatch(span);

    CHK(got == 4);
    CHK(out[0] == b);
    CHK(out[1] == a);
}

// Verifies freeBlocks() is updated once for the whole batch.
static void free_blocks_updates_after_batch() {
    Pool<> pool(sizeof(void*), 4);
    void* out[3];
    std::span<void*> span(out, 3);

    (void)pool.allocateBatch(span);
    CHK(pool.freeBlocks() == 1);
}

// Executes all allocateBatch() test cases.
static void run_tests() {
    RUN(fills_all_when_capacity_allows);
    RUN(returns_partial_count_when_exhausted);
    RUN(drains_free_list_before_watermark);
    RUN(free_blocks_updates_after_batch);
}

REGISTER_TEST_SUITE();
