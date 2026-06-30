// Pool constructor test suite.
//
// Coverage:
// - Basic construction
// - Default alignment
// - Initial state
// - Custom alignment
// - Large capacity

#include "test_helper.h"

using namespace AllocatorPro;

// Verifies that a pool initializes with the specified block count.
static void basic_construction() {
    Pool pool{64, 16};
    CHK(pool.totalBlocks() == 16);
}

// Verifies that the default alignment produces a valid stride.
static void default_alignment() {
    Pool pool{64, 16};
    CHK(pool.blockStride() == 64);
}

// Verifies that a newly constructed pool has all blocks free.
static void initial_state() {
    Pool pool{64, 16};
    CHK(pool.usedBlocks() == 0);
    CHK(pool.freeBlocks() == pool.totalBlocks());
}

// Verifies that a custom power-of-two alignment produces the correct stride.
static void custom_alignment() {
    Pool pool{10, 16, 8};
    CHK(pool.blockStride() == 16);
    CHK(pool.totalBlocks() == 16);
    CHK(pool.usedBlocks()  == 0);
}

// Verifies that a pool supports large block counts.
static void large_capacity() {
    Pool pool{64, 1024};
    CHK(pool.totalBlocks() == 1024);
    CHK(pool.usedBlocks()  == 0);
    CHK(pool.freeBlocks()  == 1024);
}

// Executes all constructor test cases.
void run_constructor_tests() {
    setTitle("Constructor");

    RUN(basic_construction);
    RUN(default_alignment);
    RUN(initial_state);
    RUN(custom_alignment);
    RUN(large_capacity);

    std::cout << "\n";
}
