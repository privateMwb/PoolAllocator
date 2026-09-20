// Pool alignment test suite.
//
// Coverage:
// - Default alignment matches alignof(std::max_align_t)
// - A custom alignment is honored by returned pointers
// - blockStride() rounds a non-aligned blockSize up correctly

#include <support/framework.h>

#include <cstdint>

using namespace PoolPro;

// Verifies the default-constructed alignment matches max_align_t.
static void default_alignment_matches_max_align() {
    Pool<> pool(sizeof(void*), 4);
    CHK(pool.blockStride() % alignof(std::max_align_t) == 0);
}

// Verifies a custom alignment request is actually honored by allocate().
static void custom_alignment_is_honored() {
    constexpr std::size_t alignment = 64;
    Pool<> pool(sizeof(void*), 4, alignment);

    void* p = pool.allocate();
    CHK(reinterpret_cast<std::uintptr_t>(p) % alignment == 0);
}

// Verifies a blockSize that isn't already a multiple of the alignment
// gets rounded up rather than truncated.
static void stride_rounds_blocksize_up() {
    Pool<> pool(24, 4, 16);
    CHK(pool.blockStride() == 32);
}

// Executes all alignment test cases.
static void run_tests() {
    RUN(default_alignment_matches_max_align);
    RUN(custom_alignment_is_honored);
    RUN(stride_rounds_blocksize_up);
}

REGISTER_TEST_SUITE();
