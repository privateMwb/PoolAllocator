// Pool destroy() test suite.
//
// Coverage:
// - Runs T's destructor
// - Reclaims the block via deallocate() so it can be reused
// - nullptr is silently ignored

#include <support/framework.h>

using namespace PoolPro;

namespace {

struct Tracked {
    bool* destroyedFlag;
    explicit Tracked(bool* flag) : destroyedFlag(flag) {
        *destroyedFlag = false;
    }
    ~Tracked() {
        *destroyedFlag = true;
    }
};

} // namespace

// Verifies destroy() invokes T's destructor.
static void runs_destructor() {
    Pool<> pool(sizeof(void*), 2);
    bool destroyed = false;
    Tracked* t = pool.create<Tracked>(&destroyed);
    CHK(t != nullptr);

    pool.destroy(t);
    CHK(destroyed == true);
}

// Verifies destroy() returns the block to the pool, unlike a bare dtor call.
static void reclaims_block() {
    Pool<> pool(sizeof(void*), 1);
    bool destroyed = false;
    Tracked* t = pool.create<Tracked>(&destroyed);
    CHK(pool.usedBlocks() == 1);

    pool.destroy(t);
    CHK(pool.usedBlocks() == 0);

    bool destroyedAgain = false;
    Tracked* reused = pool.create<Tracked>(&destroyedAgain);
    CHK(reused != nullptr);
    pool.destroy(reused);
}

// Verifies destroy(nullptr) is a no-op.
static void ignores_null_pointer() {
    Pool<> pool(sizeof(void*), 1);
    Tracked* p = nullptr;
    pool.destroy(p);
    CHK(pool.usedBlocks() == 0);
}

// Executes all destroy() test cases.
static void run_tests() {
    RUN(runs_destructor);
    RUN(reclaims_block);
    RUN(ignores_null_pointer);
}

REGISTER_TEST_SUITE();
