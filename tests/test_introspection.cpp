// Pool Introspection Test Suite
//
// Coverage:
// - Pointer ownership
// - Pool capacity
// - Block usage tracking
// - Block stride
// - Statistics tracking

#include "test_helper.h"

using namespace AllocatorPro;

// Valid Ownership
// Verifies that owns() recognizes pointers allocated by the pool.
static void owns_valid_block() {
    Pool pool{64, 4};
    void* ptr = pool.allocate();

    CHK(pool.owns(ptr));
}

// Out-of-Range Ownership
// Verifies that owns() rejects pointers outside the pool.
static void owns_out_of_range() {
    Pool pool{64, 4};
    int x = 0;

    CHK(!pool.owns(&x));
}

// Null Pointer Ownership
// Verifies that owns() returns false for a nullptr.
static void owns_nullptr() {
    Pool pool{64, 4};

    CHK(!pool.owns(nullptr));
}

// Misaligned Pointer Ownership
// Verifies that owns() rejects pointers that are not block-aligned.
static void owns_misaligned_pointer() {
    Pool pool{64, 4};
    void* ptr = pool.allocate();

    const auto* p = static_cast<const std::byte*>(ptr) + 1;

    CHK(!pool.owns(p));
}

// Capacity Reporting
// Verifies that capacity() matches the total pool memory.
static void capacity_is_correct() {
    Pool pool{64, 4};

    CHK(pool.capacity() == pool.blockStride() * pool.totalBlocks());
}

// Block Usage Tracking
// Verifies that usedBlocks() and freeBlocks() track allocations correctly.
static void used_and_free_blocks() {
    Pool pool{64, 4};

    void* a = pool.allocate();
    void* b = pool.allocate();

    CHK(pool.usedBlocks() == 2);
    CHK(pool.freeBlocks() == 2);

    pool.deallocate(a);
    pool.deallocate(b);

    CHK(pool.usedBlocks() == 0);
    CHK(pool.freeBlocks() == 4);
}

// Total Block Count
// Verifies that totalBlocks() remains unchanged after pool operations.
static void total_blocks_unchanged() {
    Pool pool{64, 4};

    (void)pool.allocate();
    (void)pool.allocate();

    CHK(pool.totalBlocks() == 4);

    pool.reset();

    CHK(pool.totalBlocks() == 4);
}

// Block Stride
// Verifies that blockStride() reports the aligned block size.
static void block_stride_is_correct() {
    Pool pool{10, 4, 8};

    CHK(pool.blockStride() == 16);
}

// Allocation Statistics
// Verifies that allocation statistics are updated correctly.
static void stats_track_allocations() {
    Pool<true> pool{64, 4};

    (void)pool.allocate();
    (void)pool.allocate();

    CHK(pool.getStats().allocations_ == 2);
    CHK(pool.getStats().totalAllocated_ == 2);
}

// Deallocation Statistics
// Verifies that deallocation statistics are updated correctly.
static void stats_track_deallocations() {
    Pool<true> pool{64, 4};

    void* a = pool.allocate();
    void* b = pool.allocate();
    pool.deallocate(a);
    pool.deallocate(b);

    CHK(pool.getStats().deallocations_ == 2);
}

// Peak Usage Statistics
// Verifies that peakUsed_ records the highest simultaneous allocation count.
static void stats_track_peak_used() {
    Pool<true> pool{64, 4};

    void* a = pool.allocate();
    void* b = pool.allocate();
    void* c = pool.allocate();
    pool.deallocate(c);

    CHK(pool.getStats().peakUsed_ == 3);
}

// Test Runner
// Executes all introspection test cases.
void run_introspection_tests() {
    setTitle("Introspection");
    
    RUN(owns_valid_block);
    RUN(owns_out_of_range);
    RUN(owns_nullptr);
    RUN(owns_misaligned_pointer);
    RUN(capacity_is_correct);
    RUN(used_and_free_blocks);
    RUN(total_blocks_unchanged);
    RUN(block_stride_is_correct);
    RUN(stats_track_allocations);
    RUN(stats_track_deallocations);
    RUN(stats_track_peak_used);

    std::cout << "\n";
}