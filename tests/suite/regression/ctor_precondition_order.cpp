// Regression suite: constructor contract.
//
// AP_PRE checks in the constructor body are plain assert() (see
// Contract.h) — a real violation aborts the process, which isn't
// something a test can safely trigger and recover from. So this suite
// does not violate preconditions directly. Instead it:
//  - Pins correct behavior at the exact boundary each precondition
//    allows, guarding against an off-by-one if a check is ever "fixed"
//    to be stricter than intended (e.g. `>=` becoming `>`).
//  - Proves the backing allocation is attempted unconditionally as part
//    of construction — which is the observable half of the ordering
//    concern: the allocation happens in the member-init list, before
//    AP_PRE runs in the body, so a genuinely unsatisfiable request
//    still throws std::bad_alloc rather than being caught by a check.

#include <support/framework.h>

using namespace PoolPro;

// Verifies blockCount == 1, the smallest value AP_PRE(blockCount > 0) allows.
static void minimum_valid_block_count_succeeds() {
    Pool<> pool(sizeof(void*), 1);
    CHK(pool.totalBlocks() == 1);
    CHK(pool.allocate() != nullptr);
}

// Verifies blockSize == sizeof(void*), the smallest value
// AP_PRE(blockSize >= sizeof(FreeNode)) allows (FreeNode holds a single
// pointer).
static void minimum_valid_block_size_succeeds() {
    Pool<> pool(sizeof(void*), 4);
    CHK(pool.blockStride() >= sizeof(void*));
    void* p = pool.allocate();
    CHK(p != nullptr);
}

// Verifies the constructor's backing allocation is attempted for real,
// regardless of precondition checks: a request far beyond any real
// system's address space throws std::bad_alloc instead of silently
// succeeding or being intercepted by a check.
static void oversized_request_throws_bad_alloc() {
    // Sized to land comfortably under sanitizer allocation-size caps
    // (e.g. ASan aborts outright, rather than throwing, above ~1 TiB)
    // while still being far larger than any real system can satisfy —
    // even accounting for the stride possibly rounding blockSize up to
    // the default alignment.
    constexpr std::size_t hugeBlockCount = std::size_t{1} << 33; // ~128 GiB worst case
    CHK_THROWS(Pool<>(sizeof(void*), hugeBlockCount), std::bad_alloc);
}

// Executes all constructor-contract test cases.
static void run_tests() {
    RUN(minimum_valid_block_count_succeeds);
    RUN(minimum_valid_block_size_succeeds);
    RUN(oversized_request_throws_bad_alloc);
}

REGISTER_TEST_SUITE();
