// Integration suite: create() + destroy() working together across full
// object lifecycles and repeated churn, as opposed to the single-call
// focus of the unit-level create()/destroy() suites.
//
// Coverage:
// - A create() -> destroy() round trip leaves the pool exactly as it
//   started, and the freed slot is reusable
// - Repeated full fill/drain churn across many rounds never leaks
//   capacity or corrupts the free list
// - Destroying only some live objects frees exactly those slots, and
//   nothing else

#include <gtest/gtest.h>

#include <PoolPro/Pool.h>

using namespace PoolPro;

namespace {

struct Widget {
    int id;
    bool* destroyedFlag;
    Widget(int i, bool* flag) : id(i), destroyedFlag(flag) {
        *destroyedFlag = false;
    }
    ~Widget() {
        *destroyedFlag = true;
    }
};

} // namespace

// Verifies a create()/destroy() round trip returns the pool to its
// starting state, with the slot ready for reuse.
TEST(CreateDestroyCycle, CreateDestroyFreesSlot) {
    Pool<> pool(sizeof(Widget), 2);
    bool destroyed = false;

    Widget* w = pool.create<Widget>(1, &destroyed);
    ASSERT_NE(w, nullptr);
    EXPECT_EQ(pool.usedBlocks(), 1u);

    pool.destroy(w);
    EXPECT_TRUE(destroyed);
    EXPECT_EQ(pool.usedBlocks(), 0u);

    bool destroyedAgain = false;
    Widget* reused = pool.create<Widget>(2, &destroyedAgain);
    ASSERT_NE(reused, nullptr);
    EXPECT_EQ(reused->id, 2);
    pool.destroy(reused);
}

// Verifies many rounds of full fill/drain churn leave capacity and free
// list state untouched between rounds.
TEST(CreateDestroyCycle, RepeatedChurnNoLeak) {
    Pool<> pool(sizeof(Widget), 4);

    for (int round = 0; round < 10; ++round) {
        Widget* items[4];
        bool destroyedFlags[4];
        for (int i = 0; i < 4; ++i) {
            items[i] = pool.create<Widget>(i, &destroyedFlags[i]);
            ASSERT_NE(items[i], nullptr);
        }
        EXPECT_EQ(pool.usedBlocks(), 4u);

        bool unusedFlag = false;
        Widget* overflow = pool.create<Widget>(99, &unusedFlag);
        EXPECT_EQ(overflow, nullptr); // exhausted

        for (int i = 0; i < 4; ++i)
            pool.destroy(items[i]);

        EXPECT_EQ(pool.usedBlocks(), 0u);
        for (int i = 0; i < 4; ++i)
            EXPECT_TRUE(destroyedFlags[i]);
    }
}

// Verifies destroying one live object only frees that object's slot,
// leaving the others untouched.
TEST(CreateDestroyCycle, PartialDestroyFreesTargetedSlot) {
    Pool<> pool(sizeof(Widget), 3);
    bool flagA = false, flagB = false, flagC = false;

    Widget* a = pool.create<Widget>(1, &flagA);
    Widget* b = pool.create<Widget>(2, &flagB);
    Widget* c = pool.create<Widget>(3, &flagC);
    EXPECT_EQ(pool.usedBlocks(), 3u);

    pool.destroy(b);
    EXPECT_TRUE(flagB);
    EXPECT_FALSE(flagA);
    EXPECT_FALSE(flagC);
    EXPECT_EQ(pool.usedBlocks(), 2u);

    bool flagD = false;
    Widget* d = pool.create<Widget>(4, &flagD);
    ASSERT_NE(d, nullptr);
    EXPECT_EQ(d, b); // reuses the one slot that was actually freed

    pool.destroy(a);
    pool.destroy(c);
    pool.destroy(d);
}
