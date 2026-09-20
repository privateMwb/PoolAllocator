// Regression suite: alignment contract.
//
// AP_PRE(isPowerOfTwo(alignment)) is a plain assert() (see Contract.h),
// so an actual violation aborts the process rather than something a
// test can safely trigger. This suite instead pins correct behavior at
// the boundaries of what's valid, and confirms blockStride() — not just
// a single returned pointer — consistently reflects the alignment
// contract across many blocks.

#include <gtest/gtest.h>

#include <PoolPro/Pool.h>

#include <cstddef>
#include <cstdint>

using namespace PoolPro;

// Verifies alignment == 1 (the smallest power of two) is accepted and
// packs blocks with no padding.
TEST(AlignmentContract, MinimumValidAlignmentSucceeds) {
    Pool<> pool(sizeof(void*), 4, 1);
    EXPECT_EQ(pool.blockStride(), sizeof(void*));
    EXPECT_NE(pool.allocate(), nullptr);
}

// Verifies a large power-of-two alignment is honored by returned pointers.
TEST(AlignmentContract, LargeAlignmentIsHonored) {
    constexpr std::size_t alignment = 256;
    Pool<> pool(sizeof(void*), 4, alignment);

    void* p = pool.allocate();
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(p) % alignment, 0u);
}

// Verifies blockStride() — not just the first pointer — consistently
// reflects the alignment contract: every block in a multi-block pool
// must land on an alignment boundary, not just block 0.
TEST(AlignmentContract, EveryBlockInPoolHonorsAlignment) {
    constexpr std::size_t alignment = 32;
    Pool<> pool(sizeof(void*), 5, alignment);

    for (int i = 0; i < 5; ++i) {
        void* p = pool.allocate();
        EXPECT_NE(p, nullptr);
        EXPECT_EQ(reinterpret_cast<std::uintptr_t>(p) % alignment, 0u);
    }
}
