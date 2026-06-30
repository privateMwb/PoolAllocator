// Pool introspection test suite.
//
// Coverage:
// - Owns returns true for allocated pointer
// - Owns returns false for misaligned interior pointer
// - Owns returns false for outside pointer
// - Owns returns false for nullptr
// - UsedBlocks reflects current allocation
// - FreeBlocks reflects available blocks
// - TotalBlocks reflects block count
// - UsedBlocks and freeBlocks sum to totalBlocks
// - Capacity reflects stride times total blocks
// - BlockStride reflects aligned block size

#include "test_helper.h"

using namespace AllocatorPro;

// Verifies that owns returns true for a pointer returned by allocate.
static void owns_returns_true_for_allocated_pointer() {
    Pool pool{64, 4};
    void* ptr = pool.allocate();
    CHK(pool.owns(ptr));
}

// Verifies that owns returns false for a misaligned interior pointer.
static void owns_returns_false_for_interior_pointer() {
    Pool pool{64, 4};
    void* ptr = pool.allocate();
    const auto* interior = static_cast<const std::byte*>(ptr) + 1;
    CHK(!pool.owns(interior));
}

// Verifies that owns returns false for a pointer outside the pool's slab.
static void owns_returns_false_for_outside_pointer() {
    Pool pool{64, 4};
    int x = 0;
    CHK(!pool.owns(&x));
}

// Verifies that owns returns false for a null pointer.
static void owns_returns_false_for_nullptr() {
    Pool pool{64, 4};
    CHK(!pool.owns(nullptr));
}

// Verifies that usedBlocks reflects the current number of allocated blocks.
static void used_blocks_reflects_current_allocation() {
    Pool pool{64, 4};
    (void)pool.allocate();
    (void)pool.allocate();
    CHK(pool.usedBlocks() == 2);
}

// Verifies that freeBlocks reflects the number of available blocks.
static void free_blocks_reflects_available_capacity() {
    Pool pool{64, 4};
    (void)pool.allocate();
    CHK(pool.freeBlocks() == 3);
}

// Verifies that totalBlocks reflects the total block count.
static void total_blocks_reflects_block_count() {
    Pool pool{64, 4};
    CHK(pool.totalBlocks() == 4);
}

// Verifies that used and free blocks always sum to total blocks.
static void used_and_free_sum_to_total() {
    Pool pool{64, 4};
    (void)pool.allocate();
    (void)pool.allocate();
    CHK(pool.usedBlocks() + pool.freeBlocks() == pool.totalBlocks());
}

// Verifies that capacity reflects stride multiplied by total blocks.
static void capacity_reflects_stride_times_total() {
    Pool pool{64, 4};
    CHK(pool.capacity() == pool.blockStride() * pool.totalBlocks());
}

// Verifies that blockStride reflects the alignment-padded block size.
static void block_stride_reflects_aligned_size() {
    Pool pool{10, 4, 8};
    CHK(pool.blockStride() == 16);
}

// Executes all introspection test cases.
void run_introspection_tests() {
    setTitle("Introspection");

    RUN(owns_returns_true_for_allocated_pointer);
    RUN(owns_returns_false_for_interior_pointer);
    RUN(owns_returns_false_for_outside_pointer);
    RUN(owns_returns_false_for_nullptr);
    RUN(used_blocks_reflects_current_allocation);
    RUN(free_blocks_reflects_available_capacity);
    RUN(total_blocks_reflects_block_count);
    RUN(used_and_free_sum_to_total);
    RUN(capacity_reflects_stride_times_total);
    RUN(block_stride_reflects_aligned_size);

    std::cout << "\n";
}
