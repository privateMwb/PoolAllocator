// Regression suite: alignment contract.
//
// AP_PRE(isPowerOfTwo(alignment)) is a plain assert() (see Contract.h),
// so an actual violation aborts the process rather than something a
// test can safely trigger. This suite instead pins correct behavior at
// the boundaries of what's valid, and confirms blockStride() — not just
// a single returned pointer — consistently reflects the alignment
// contract across many blocks.

#include <support/framework.h>

#include <cstdint>

using namespace PoolPro;

// Verifies alignment == 1 (the smallest power of two) is accepted and
// packs blocks with no padding.
static void minimum_valid_alignment_succeeds() {
    Pool<> pool(sizeof(void*), 4, 1);
    CHK(pool.blockStride() == sizeof(void*));
    CHK(pool.allocate() != nullptr);
}

// Verifies a large power-of-two alignment is honored by returned pointers.
static void large_alignment_is_honored() {
    constexpr std::size_t alignment = 256;
    Pool<> pool(sizeof(void*), 4, alignment);

    void* p = pool.allocate();
    CHK(reinterpret_cast<std::uintptr_t>(p) % alignment == 0);
}

// Verifies blockStride() — not just the first pointer — consistently
// reflects the alignment contract: every block in a multi-block pool
// must land on an alignment boundary, not just block 0.
static void every_block_in_pool_honors_alignment() {
    constexpr std::size_t alignment = 32;
    Pool<> pool(sizeof(void*), 5, alignment);

    for (int i = 0; i < 5; ++i) {
        void* p = pool.allocate();
        CHK(p != nullptr);
        CHK(reinterpret_cast<std::uintptr_t>(p) % alignment == 0);
    }
}

// Executes all alignment-contract test cases.
static void run_tests() {
    RUN(minimum_valid_alignment_succeeds);
    RUN(large_alignment_is_honored);
    RUN(every_block_in_pool_honors_alignment);
}

REGISTER_TEST_SUITE();
