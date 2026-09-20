// Pool alignment test suite.
//
// Coverage:
// - Default alignment matches alignof(std::max_align_t)
// - A custom alignment is honored by returned pointers
// - blockStride() rounds a non-aligned blockSize up correctly

#include <gtest/gtest.h>

#include <PoolPro/Pool.h>

#include <cstddef>
#include <cstdint>

using namespace PoolPro;

// Verifies the default-constructed alignment matches max_align_t.
TEST(Alignment, DefaultAlignmentMatchesMaxAlign) {
    Pool<> pool(sizeof(void*), 4);
    EXPECT_EQ(pool.blockStride() % alignof(std::max_align_t), 0u);
}

// Verifies a custom alignment request is actually honored by allocate().
TEST(Alignment, CustomAlignmentIsHonored) {
    constexpr std::size_t alignment = 64;
    Pool<> pool(sizeof(void*), 4, alignment);

    void* p = pool.allocate();
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(p) % alignment, 0u);
}

// Verifies a blockSize that isn't already a multiple of the alignment
// gets rounded up rather than truncated.
TEST(Alignment, StrideRoundsBlockSizeUp) {
    Pool<> pool(24, 4, 16);
    EXPECT_EQ(pool.blockStride(), 32u);
}
