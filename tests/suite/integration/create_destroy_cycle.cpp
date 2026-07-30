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

#include <support/framework.h>

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
static void create_then_destroy_frees_slot_for_reuse() {
    Pool<> pool(sizeof(Widget), 2);
    bool destroyed = false;

    Widget* w = pool.create<Widget>(1, &destroyed);
    CHK(w != nullptr);
    CHK(pool.usedBlocks() == 1);

    pool.destroy(w);
    CHK(destroyed == true);
    CHK(pool.usedBlocks() == 0);

    bool destroyedAgain = false;
    Widget* reused = pool.create<Widget>(2, &destroyedAgain);
    CHK(reused != nullptr);
    CHK(reused->id == 2);
    pool.destroy(reused);
}

// Verifies many rounds of full fill/drain churn leave capacity and free
// list state untouched between rounds.
static void repeated_churn_never_leaks_capacity() {
    Pool<> pool(sizeof(Widget), 4);

    for (int round = 0; round < 10; ++round) {
        Widget* items[4];
        bool destroyedFlags[4];
        for (int i = 0; i < 4; ++i) {
            items[i] = pool.create<Widget>(i, &destroyedFlags[i]);
            CHK(items[i] != nullptr);
        }
        CHK(pool.usedBlocks() == 4);

        bool unusedFlag = false;
        CHK(pool.create<Widget>(99, &unusedFlag) == nullptr); // exhausted

        for (int i = 0; i < 4; ++i)
            pool.destroy(items[i]);

        CHK(pool.usedBlocks() == 0);
        for (int i = 0; i < 4; ++i)
            CHK(destroyedFlags[i] == true);
    }
}

// Verifies destroying one live object only frees that object's slot,
// leaving the others untouched.
static void partial_destroy_frees_only_targeted_slot() {
    Pool<> pool(sizeof(Widget), 3);
    bool flagA = false, flagB = false, flagC = false;

    Widget* a = pool.create<Widget>(1, &flagA);
    Widget* b = pool.create<Widget>(2, &flagB);
    Widget* c = pool.create<Widget>(3, &flagC);
    CHK(pool.usedBlocks() == 3);

    pool.destroy(b);
    CHK(flagB == true);
    CHK(flagA == false);
    CHK(flagC == false);
    CHK(pool.usedBlocks() == 2);

    bool flagD = false;
    Widget* d = pool.create<Widget>(4, &flagD);
    CHK(d != nullptr);
    CHK(d == b); // reuses the one slot that was actually freed

    pool.destroy(a);
    pool.destroy(c);
    pool.destroy(d);
}

// Executes all create()/destroy() cycle test cases.
static void run_tests() {
    RUN(create_then_destroy_frees_slot_for_reuse);
    RUN(repeated_churn_never_leaks_capacity);
    RUN(partial_destroy_frees_only_targeted_slot);
}

REGISTER_TEST_SUITE();
