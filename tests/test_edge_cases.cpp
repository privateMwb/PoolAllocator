// Pool Edge Cases Test Suite
//
// Coverage:
// - Empty pool allocation
// - Foreign pointer deallocation
// - Ownership after move
// - Reset after exhaustion
// - Batch allocation from empty pool
// - Batch deallocation with nullptr entries
// - Single-block pool behavior

#include "test_helper.h"

using namespace AllocatorPro;

// Empty Pool Allocation
// Verifies that allocate() returns nullptr when the pool is exhausted.
static void allocate_from_empty_pool() {
    Pool pool{64, 4};

    (void)pool.allocate();
    (void)pool.allocate();
    (void)pool.allocate();
    (void)pool.allocate();

    CHK(pool.allocate() == nullptr);
    CHK(pool.freeBlocks() == 0);
}

// Foreign Pointer Deallocation
// Verifies that deallocating a pointer outside the pool has no effect.
static void deallocate_foreign_pointer() {
    Pool pool{64, 4};
    int x = 0;

    pool.deallocate(&x);

    CHK(pool.freeBlocks() == 4);
}

// Ownership After Move
// Verifies that ownership transfers correctly after move construction.
static void owns_after_move() {
    Pool src{64, 4};
    void* ptr = src.allocate();

    Pool dst{std::move(src)};

    CHK(dst.owns(ptr));
    CHK(!src.owns(ptr));
}

// Reset After Exhaustion
// Verifies that reset() restores the pool after all blocks are allocated.
static void full_pool_reset_reallocate() {
    Pool pool{64, 4};

    (void)pool.allocate();
    (void)pool.allocate();
    (void)pool.allocate();
    (void)pool.allocate();

    CHK(pool.freeBlocks() == 0);

    pool.reset();

    CHK(pool.freeBlocks() == 4);
    CHK(pool.allocate() != nullptr);
}

// Batch Allocation From Empty Pool
// Verifies that batch allocation returns zero when no blocks are available.
static void batch_allocate_empty_pool() {
    Pool pool{64, 4};

    (void)pool.allocate();
    (void)pool.allocate();
    (void)pool.allocate();
    (void)pool.allocate();

    void* blocks[4]{};
    std::size_t count = pool.allocateBatch(blocks);

    CHK(count == 0);
}

// Batch Deallocation With Null Entries
// Verifies that nullptr entries are safely ignored during batch deallocation.
static void batch_deallocate_with_nullptr_entries() {
    Pool pool{64, 4};

    void* blocks[4]{};
    blocks[0] = pool.allocate();
    blocks[1] = nullptr;
    blocks[2] = pool.allocate();
    blocks[3] = nullptr;

    pool.deallocateBatch(blocks);

    CHK(pool.freeBlocks() == 4);
}

// Single-Block Pool
// Verifies allocator behavior when the pool contains only one block.
static void single_block_pool() {
    Pool pool{64, 1};

    void* ptr = pool.allocate();

    CHK(ptr != nullptr);
    CHK(pool.freeBlocks() == 0);
    CHK(pool.allocate() == nullptr);

    pool.deallocate(ptr);

    CHK(pool.freeBlocks() == 1);
}

// Test Runner
// Executes all edge case test cases.
void run_edge_cases_tests() {
    setTitle("Edge Cases");
    
    RUN(allocate_from_empty_pool);
    RUN(deallocate_foreign_pointer);
    RUN(owns_after_move);
    RUN(full_pool_reset_reallocate);
    RUN(batch_allocate_empty_pool);
    RUN(batch_deallocate_with_nullptr_entries);
    RUN(single_block_pool);

    std::cout << "\n";
}