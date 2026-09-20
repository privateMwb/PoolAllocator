// Pool stats test suite (EnableStats == true).
//
// Coverage:
// - totalAllocated_/allocations_/deallocations_ track individual calls
// - peakUsed_ tracks the high-water mark and never drops on deallocate()
// - A batch call updates stats for every block it touches, in one shot
// - reset() zeroes accumulated stats

#include <support/framework.h>

using namespace PoolPro;

// Verifies allocate()/deallocate() increment the matching counters.
static void tracks_allocations_and_deallocations() {
    Pool<true> pool(sizeof(void*), 4);
    void* a = pool.allocate();
    void* b = pool.allocate();
    CHK(pool.getStats().totalAllocated_ == 2);
    CHK(pool.getStats().allocations_ == 2);

    pool.deallocate(a);
    pool.deallocate(b);
    CHK(pool.getStats().deallocations_ == 2);
}

// Verifies peakUsed_ records the high-water mark and isn't reduced by frees.
static void tracks_peak_used() {
    Pool<true> pool(sizeof(void*), 4);
    void* a = pool.allocate();
    void* b = pool.allocate();
    CHK(pool.getStats().peakUsed_ == 2);

    pool.deallocate(a);
    pool.deallocate(b);
    CHK(pool.getStats().peakUsed_ == 2);
}

// Verifies allocateBatch() accounts for every block in the batch, not
// just a single call.
static void batch_updates_stats_for_all_blocks() {
    Pool<true> pool(sizeof(void*), 4);
    void* out[3];
    std::span<void*> span(out, 3);

    std::size_t got = pool.allocateBatch(span);
    CHK(got == 3);
    CHK(pool.getStats().allocations_ == 3);
    CHK(pool.getStats().totalAllocated_ == 3);
    CHK(pool.getStats().peakUsed_ == 3);
}

// Verifies reset() clears accumulated stats back to zero.
static void reset_clears_stats() {
    Pool<true> pool(sizeof(void*), 4);
    (void)pool.allocate();

    pool.reset();
    const auto& stats = pool.getStats();
    CHK(stats.totalAllocated_ == 0);
    CHK(stats.allocations_ == 0);
    CHK(stats.peakUsed_ == 0);
}

// Executes all stats test cases.
static void run_tests() {
    RUN(tracks_allocations_and_deallocations);
    RUN(tracks_peak_used);
    RUN(batch_updates_stats_for_all_blocks);
    RUN(reset_clears_stats);
}

REGISTER_TEST_SUITE();
