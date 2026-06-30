// Pool Memory Allocation Test Suite
//
// Coverage:
// - Single block allocation
// - Allocation until pool exhaustion
// - Block deallocation
// - Null pointer deallocation
// - Pointer reuse after deallocation
// - Batch allocation
// - Partial batch allocation
// - Batch deallocation

#include "test_helper.h"

using namespace AllocatorPro;

// Verifies that allocating a single block updates the pool state correctly.
static void single_allocate() {
    Pool pool{64, 4};
    void* ptr = pool.allocate();

    CHK(ptr != nullptr);
    CHK(pool.usedBlocks() == 1);
    CHK(pool.freeBlocks() == 3);
}

// Verifies that allocation returns nullptr after all blocks have been consumed.
static void allocate_until_exhausted() {
    Pool pool{64, 4};

    (void)pool.allocate();
    (void)pool.allocate();
    (void)pool.allocate();
    (void)pool.allocate();

    CHK(pool.freeBlocks() == 0);
    CHK(pool.usedBlocks() == 4);
    CHK(pool.allocate() == nullptr);
}

// Verifies that deallocating a block returns it to the pool.
static void deallocate_returns_block() {
    Pool pool{64, 4};
    void* ptr = pool.allocate();

    pool.deallocate(ptr);

    CHK(pool.usedBlocks() == 0);
    CHK(pool.freeBlocks() == 4);
}

// Verifies that deallocating a nullptr is a safe no-op.
static void deallocate_nullptr() {
    Pool pool{64, 4};
    pool.deallocate(nullptr);

    CHK(pool.usedBlocks() == 0);
}

// Verifies that a freed block is reused by the next allocation.
static void pointer_reuse_after_deallocate() {
    Pool pool{64, 4};
    void* first = pool.allocate();
    pool.deallocate(first);
    void* second = pool.allocate();

    CHK(first == second);
}

// Verifies that all available blocks can be allocated in a single batch.
static void batch_allocate() {
    Pool pool{64, 8};
    void* blocks[8]{};
    std::size_t count = pool.allocateBatch(blocks);

    CHK(count == 8);
    CHK(pool.freeBlocks() == 0);
    CHK(pool.usedBlocks() == 8);
}

// Verifies that batch allocation stops when the pool becomes exhausted.
static void batch_allocate_partial() {
    Pool pool{64, 4};
    void* blocks[8]{};
    std::size_t count = pool.allocateBatch(blocks);

    CHK(count == 4);
    CHK(pool.freeBlocks() == 0);
}

// Verifies that multiple blocks can be returned to the pool in a single operation.
static void batch_deallocate() {
    Pool pool{64, 8};
    void* blocks[8]{};
    (void)pool.allocateBatch(blocks);
    pool.deallocateBatch(blocks);

    CHK(pool.freeBlocks() == 8);
    CHK(pool.usedBlocks() == 0);
}

// Executes all memory allocation test cases.
void run_memory_tests() {
    setTitle("Memory Allocation");

    RUN(single_allocate);
    RUN(allocate_until_exhausted);
    RUN(deallocate_returns_block);
    RUN(deallocate_nullptr);
    RUN(pointer_reuse_after_deallocate);
    RUN(batch_allocate);
    RUN(batch_allocate_partial);
    RUN(batch_deallocate);

    std::cout << "\n";
}