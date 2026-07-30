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

#include <support/framework.h>

using namespace PoolPro;

// Verifies a single allocateBatch() call correctly stitches together
// free-list reuse and watermark advancement when both are needed.
static void batch_drains_partial_freelist_then_advances_watermark() {
    Pool<> pool(sizeof(void*), 8);

    void* blocks[4];
    for (int i = 0; i < 4; ++i)
        blocks[i] = pool.allocate();
    CHK(pool.usedBlocks() == 4);
    CHK(pool.freeBlocks() == 4); // 4 blocks still virgin

    // Free two of the four live blocks; free list is LIFO, so the most
    // recently freed one comes back out first.
    pool.deallocate(blocks[2]);
    pool.deallocate(blocks[0]);
    CHK(pool.freeBlocks() == 6); // 2 free-listed + 4 virgin

    void* out[6];
    std::span<void*> span(out, 6);
    std::size_t got = pool.allocateBatch(span);

    CHK(got == 6);
    CHK(out[0] == blocks[0]);
    CHK(out[1] == blocks[2]);
    for (int i = 2; i < 6; ++i) {
        for (int j = 2; j < i; ++j)
            CHK(out[i] != out[j]); // the virgin portion is all distinct
    }

    CHK(pool.usedBlocks() == 8);
    CHK(pool.freeBlocks() == 0);
}

// Verifies a batch is satisfied entirely from the free list once the
// watermark has nothing left to give.
static void batch_draws_entirely_from_freelist_when_watermark_exhausted() {
    Pool<> pool(sizeof(void*), 4);
    void* blocks[4];
    for (int i = 0; i < 4; ++i)
        blocks[i] = pool.allocate();
    CHK(pool.freeBlocks() == 0); // watermark fully advanced

    pool.deallocateBatch(std::span<void*>(blocks, 4));
    CHK(pool.freeBlocks() == 4);

    void* out[4];
    std::span<void*> span(out, 4);
    std::size_t got = pool.allocateBatch(span);
    CHK(got == 4);
    CHK(pool.freeBlocks() == 0);
}

// Verifies a batch is satisfied entirely from virgin memory when nothing
// has ever been freed.
static void batch_draws_entirely_from_virgin_memory_when_freelist_empty() {
    Pool<> pool(sizeof(void*), 5);
    void* out[5];
    std::span<void*> span(out, 5);

    std::size_t got = pool.allocateBatch(span);
    CHK(got == 5);
    CHK(pool.usedBlocks() == 5);
}

// Executes all batch/free-list interaction test cases.
static void run_tests() {
    RUN(batch_drains_partial_freelist_then_advances_watermark);
    RUN(batch_draws_entirely_from_freelist_when_watermark_exhausted);
    RUN(batch_draws_entirely_from_virgin_memory_when_freelist_empty);
}

REGISTER_TEST_SUITE();
