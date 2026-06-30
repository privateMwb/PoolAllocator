// Pool move semantics test suite.
//
// Coverage:
// - Move constructor
// - Move assignment
// - Self move-assignment

#include "test_helper.h"

using namespace AllocatorPro;

// Verifies that move construction transfers ownership of the allocator state.
static void move_constructor() {
    Pool src{64, 16};
    void* ptr = src.allocate();

    Pool dst{std::move(src)};

    CHK(dst.totalBlocks() == 16);
    CHK(dst.usedBlocks()  == 1);
    CHK(dst.owns(ptr));
    CHK(src.totalBlocks() == 0);
    CHK(src.usedBlocks()  == 0);
}

// Verifies that move assignment transfers ownership of the allocator state.
static void move_assignment() {
    Pool src{64, 16};
    Pool dst{32, 8};

    void* ptr = src.allocate();
    dst = std::move(src);

    CHK(dst.totalBlocks() == 16);
    CHK(dst.usedBlocks()  == 1);
    CHK(dst.owns(ptr));
    CHK(src.totalBlocks() == 0);
    CHK(src.usedBlocks()  == 0);
}

// Verifies that self move-assignment preserves a valid allocator state.
static void move_self_assignment() {
    Pool pool{64, 16};
    pool.allocate();

    pool = std::move(pool);

    CHK(pool.totalBlocks() == 16);
    CHK(pool.usedBlocks()  == 1);
}

// Executes all move semantics test cases.
void run_move_tests() {
    setTitle("Move Semantics");

    RUN(move_constructor);
    RUN(move_assignment);
    RUN(move_self_assignment);

    std::cout << "\n";
}
