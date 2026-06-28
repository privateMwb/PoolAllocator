// Pool Constructor Test Suite
//
// Coverage:
// - Basic construction
// - Default alignment
// - Block stride computation
// - Initial pool state
// - Move constructor
// - Move assignment
// - Self move-assignment

#include "test_helper.h"

using namespace AllocatorPro;

// Basic Construction
// Verifies that a newly constructed pool initializes with the expected configuration.
static void basic_construction() {
	Pool pool{64, 16};
	CHK(pool.totalBlocks() == 16);
	CHK(pool.freeBlocks()  == 16);
	CHK(pool.usedBlocks()  == 0);
}

// Default Alignment
// Verifies that the default alignment produces the expected block stride.
static void default_alignment() {
	Pool pool{64, 16};
	CHK(pool.blockStride() == 64);
}

// Stride Computation
// Verifies that block stride is correctly aligned to the requested alignment.
static void stride_computation() {
	Pool pool{10, 16, 8};
	CHK(pool.blockStride() == 16);
}

// Initial State
// Verifies that a newly created pool starts with all blocks available.
static void initial_state() {
	Pool pool{64, 16};
	CHK(pool.freeBlocks()  == pool.totalBlocks());
	CHK(pool.usedBlocks()  == 0);
	CHK(pool.capacity()    == pool.blockStride() * pool.totalBlocks());
}

// Move Constructor
// Verifies that ownership of the pool transfers correctly during move construction.
static void move_constructor() {
	Pool src{64, 16};
	void* ptr = src.allocate();

	Pool dst{std::move(src)};

	CHK(dst.usedBlocks()  == 1);
	CHK(dst.owns(ptr));
	CHK(src.totalBlocks() == 0);
	CHK(src.freeBlocks()  == 0);
}

// Move Assignment
// Verifies that ownership of the pool transfers correctly during move assignment.
static void move_assignment() {
	Pool src{64, 16};
	Pool dst{32, 8};

	void* ptr = src.allocate();
	dst = std::move(src);

	CHK(dst.usedBlocks()  == 1);
	CHK(dst.owns(ptr));
	CHK(src.totalBlocks() == 0);
}

// Self Move Assignment
// Verifies that self move-assignment leaves the pool in a valid state.
static void move_self_assignment() {
	Pool pool{64, 16};
	(void)pool.allocate();

	pool = std::move(pool);

	CHK(pool.totalBlocks() == 16);
	CHK(pool.usedBlocks()  == 1);
}

// Test Runner
// Executes all constructor test cases.
void run_constructor_tests() {
	setTitle("Constructor");

	RUN(basic_construction);
	RUN(default_alignment);
	RUN(stride_computation);
	RUN(initial_state);
	RUN(move_constructor);
	RUN(move_assignment);
	RUN(move_self_assignment);

	std::cout << "\n";
}