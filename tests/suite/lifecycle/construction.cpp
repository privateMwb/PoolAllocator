// Pool construction test suite.
//
// Coverage:
// - A newly constructed pool starts fully available (no blocks used)
// - totalBlocks() matches the requested block count
// - The pool is immediately usable — no lazy per-block setup is needed
//   before the first allocate()

#include <support/framework.h>

using namespace PoolPro;

// Verifies a fresh pool has every block free and none used.
static void starts_fully_available() {
    Pool<> pool(sizeof(void*), 6);
    CHK(pool.usedBlocks() == 0);
    CHK(pool.freeBlocks() == 6);
}

// Verifies totalBlocks() reflects exactly what was requested.
static void block_count_matches_request() {
    Pool<> pool(sizeof(void*), 9);
    CHK(pool.totalBlocks() == 9);
}

// Verifies every block is allocatable right after construction, with no
// warm-up pass required.
static void immediately_usable_after_construction() {
    Pool<> pool(sizeof(void*), 3);
    for (std::size_t i = 0; i < 3; ++i) {
        void* p = pool.allocate();
        CHK(p != nullptr);
    }
    CHK(pool.allocate() == nullptr);
}

// Executes all construction test cases.
static void run_tests() {
    RUN(starts_fully_available);
    RUN(block_count_matches_request);
    RUN(immediately_usable_after_construction);
}

REGISTER_TEST_SUITE();
