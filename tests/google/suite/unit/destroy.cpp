// Pool destroy() test suite.
//
// Coverage:
// - Runs T's destructor
// - Reclaims the block via deallocate() so it can be reused
// - nullptr is silently ignored

#include <gtest/gtest.h>

#include <PoolPro/Pool.h>

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
TEST(Destroy, RunsDestructor) {
    Pool<> pool(sizeof(void*), 2);
    bool destroyed = false;
    Tracked* t = pool.create<Tracked>(&destroyed);
    ASSERT_NE(t, nullptr);

    pool.destroy(t);
    EXPECT_TRUE(destroyed);
}

// Verifies destroy() returns the block to the pool, unlike a bare dtor call.
TEST(Destroy, ReclaimsBlock) {
    Pool<> pool(sizeof(void*), 1);
    bool destroyed = false;
    Tracked* t = pool.create<Tracked>(&destroyed);
    EXPECT_EQ(pool.usedBlocks(), 1u);

    pool.destroy(t);
    EXPECT_EQ(pool.usedBlocks(), 0u);

    bool destroyedAgain = false;
    Tracked* reused = pool.create<Tracked>(&destroyedAgain);
    ASSERT_NE(reused, nullptr);
    pool.destroy(reused);
}

// Verifies destroy(nullptr) is a no-op.
TEST(Destroy, IgnoresNullPointer) {
    Pool<> pool(sizeof(void*), 1);
    Tracked* p = nullptr;
    pool.destroy(p);
    EXPECT_EQ(pool.usedBlocks(), 0u);
}
