// Pool reset() test suite.
//
// Coverage:
// - Restores full availability (clears free list and watermark)
// - capacity()/totalBlocks() are unchanged by reset()
// - Allocation resumes from block 0 after a reset
// - Stats are cleared when EnableStats is set

#include <support/framework.h>

using namespace PoolPro;

// Verifies reset() clears both used blocks and the free list, making
// every block available again.
static void restores_full_availability() {
    Pool<> pool(sizeof(void*), 4);
    (void)pool.allocate();
    (void)pool.allocate();
    CHK(pool.usedBlocks() == 2);

    pool.reset();
    CHK(pool.usedBlocks() == 0);
    CHK(pool.freeBlocks() == 4);
}

// Verifies reset() does not shrink or resize the pool itself.
static void capacity_unchanged_after_reset() {
    Pool<> pool(sizeof(void*), 4);
    const std::size_t before = pool.capacity();

    (void)pool.allocate();
    pool.reset();

    CHK(pool.capacity() == before);
    CHK(pool.totalBlocks() == 4);
}

// Verifies allocation resumes from the first block after a reset,
// confirming the watermark itself was rewound, not just the free list.
static void reset_reuses_from_start() {
    Pool<> pool(sizeof(void*), 4);
    void* first = pool.allocate();
    (void)pool.allocate();

    pool.reset();
    void* afterReset = pool.allocate();
    CHK(afterReset == first);
}

// Verifies reset() zeroes out accumulated stats when stats are enabled.
static void clears_stats_when_enabled() {
    Pool<true> pool(sizeof(void*), 4);
    (void)pool.allocate();
    (void)pool.allocate();
    CHK(pool.getStats().totalAllocated_ == 2);

    pool.reset();
    const auto& stats = pool.getStats();
    CHK(stats.totalAllocated_ == 0);
    CHK(stats.peakUsed_ == 0);
    CHK(stats.allocations_ == 0);
    CHK(stats.deallocations_ == 0);
}

// Executes all reset() test cases.
static void run_tests() {
    RUN(restores_full_availability);
    RUN(capacity_unchanged_after_reset);
    RUN(reset_reuses_from_start);
    RUN(clears_stats_when_enabled);
}

REGISTER_TEST_SUITE();
