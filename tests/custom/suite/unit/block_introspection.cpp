// Pool block-introspection test suite: capacity(), usedBlocks(),
// freeBlocks(), totalBlocks(), blockStride().
//
// Coverage:
// - totalBlocks() matches the count given at construction
// - blockStride() is at least the requested block size and alignment-rounded
// - capacity() equals blockStride() * totalBlocks()
// - usedBlocks() + freeBlocks() always equals totalBlocks()

#include <support/framework.h>

using namespace PoolPro;

// Verifies totalBlocks() reflects the constructor argument.
static void total_blocks_matches_construction() {
    Pool<> pool(sizeof(void*), 7);
    CHK(pool.totalBlocks() == 7);
}

// Verifies blockStride() is large enough to hold a block and satisfies
// the requested alignment.
static void block_stride_at_least_block_size() {
    Pool<> pool(24, 4, 16);
    CHK(pool.blockStride() >= 24);
    CHK(pool.blockStride() % 16 == 0);
}

// Verifies capacity() is derived from stride and block count, not tracked
// independently.
static void capacity_equals_stride_times_total() {
    Pool<> pool(sizeof(void*), 5);
    CHK(pool.capacity() == pool.blockStride() * pool.totalBlocks());
}

// Verifies used and free block counts always sum to the total, through
// a mix of allocation and deallocation.
static void used_and_free_sum_to_total() {
    Pool<> pool(sizeof(void*), 4);
    void* a = pool.allocate();
    (void)pool.allocate();
    CHK(pool.usedBlocks() + pool.freeBlocks() == pool.totalBlocks());

    pool.deallocate(a);
    CHK(pool.usedBlocks() + pool.freeBlocks() == pool.totalBlocks());
}

// Executes all block-introspection test cases.
static void run_tests() {
    RUN(total_blocks_matches_construction);
    RUN(block_stride_at_least_block_size);
    RUN(capacity_equals_stride_times_total);
    RUN(used_and_free_sum_to_total);
}

REGISTER_TEST_SUITE();
