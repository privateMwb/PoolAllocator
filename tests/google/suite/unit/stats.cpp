// Pool stats test suite (EnableStats == true).
//
// Coverage:
// - totalAllocated_/allocations_/deallocations_ track individual calls
// - peakUsed_ tracks the high-water mark and never drops on deallocate()
// - A batch call updates stats for every block it touches, in one shot
// - reset() zeroes accumulated stats

#include <gtest/gtest.h>

#include <PoolPro/Pool.h>

#include <cstddef>
#include <span>

using namespace PoolPro;

// Verifies allocate()/deallocate() increment the matching counters.
TEST(Stats, TracksAllocationsAndDeallocations) {
    Pool<true> pool(sizeof(void*), 4);
    void* a = pool.allocate();
    void* b = pool.allocate();
    EXPECT_EQ(pool.getStats().totalAllocated_, 2u);
    EXPECT_EQ(pool.getStats().allocations_, 2u);

    pool.deallocate(a);
    pool.deallocate(b);
    EXPECT_EQ(pool.getStats().deallocations_, 2u);
}

// Verifies peakUsed_ records the high-water mark and isn't reduced by frees.
TEST(Stats, TracksPeakUsed) {
    Pool<true> pool(sizeof(void*), 4);
    void* a = pool.allocate();
    void* b = pool.allocate();
    EXPECT_EQ(pool.getStats().peakUsed_, 2u);

    pool.deallocate(a);
    pool.deallocate(b);
    EXPECT_EQ(pool.getStats().peakUsed_, 2u);
}

// Verifies allocateBatch() accounts for every block in the batch, not
// just a single call.
TEST(Stats, BatchUpdatesStatsForAllBlocks) {
    Pool<true> pool(sizeof(void*), 4);
    void* out[3];
    std::span<void*> span(out, 3);

    std::size_t got = pool.allocateBatch(span);
    EXPECT_EQ(got, 3u);
    EXPECT_EQ(pool.getStats().allocations_, 3u);
    EXPECT_EQ(pool.getStats().totalAllocated_, 3u);
    EXPECT_EQ(pool.getStats().peakUsed_, 3u);
}

// Verifies reset() clears accumulated stats back to zero.
TEST(Stats, ResetClearsStats) {
    Pool<true> pool(sizeof(void*), 4);
    (void)pool.allocate();

    pool.reset();
    const auto& stats = pool.getStats();
    EXPECT_EQ(stats.totalAllocated_, 0u);
    EXPECT_EQ(stats.allocations_, 0u);
    EXPECT_EQ(stats.peakUsed_, 0u);
}
