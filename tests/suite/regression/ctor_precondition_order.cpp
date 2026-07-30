// Regression suite: constructor contract.
//
// AP_PRE checks in the constructor body are plain assert() (see
// Contract.h) — a real violation aborts the process, which isn't
// something a test can safely trigger and recover from. So this suite
// does not violate preconditions directly. Instead it pins correct
// behavior at the exact boundary each precondition allows, guarding
// against an off-by-one if a check is ever "fixed" to be stricter than
// intended (e.g. `>=` becoming `>`).
//
// Note: this file originally also tried to prove the backing allocation
// happens unconditionally in the member-init list (before AP_PRE runs
// in the body) by requesting an unsatisfiably large allocation and
// expecting std::bad_alloc. Under ASan's default configuration, any
// allocation request it can't satisfy aborts the process directly
// rather than letting operator new return null / throw — a request
// under ASan's hard size cap triggers "out of memory: ABORTING", and
// one above it triggers "exceeds maximum supported size: ABORTING".
// Neither path is catchable, so that scenario isn't testable here
// without an environment-level override (ASAN_OPTIONS=allocator_may_
// return_null=1) that this test suite shouldn't assume is set.

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

// Executes all constructor-contract test cases.
static void run_tests() {
    RUN(minimum_valid_block_count_succeeds);
    RUN(minimum_valid_block_size_succeeds);
}

REGISTER_TEST_SUITE();
