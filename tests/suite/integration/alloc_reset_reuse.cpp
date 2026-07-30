// Integration suite: allocate() + reset() working together across full
// fill/drain cycles.
//
// Coverage:
// - Filling to capacity, resetting, and refilling reuses the exact same
//   addresses as the first pass (watermark truly rewinds, not just the
//   used-block counter)
// - reset() after a partial fill discards free-list state, not just
//   unused watermark capacity
// - The pool survives many repeated fill/reset cycles without drift

#include <support/framework.h>

using namespace PoolPro;

// Verifies a second fill after reset() lines up address-for-address with
// the first fill.
static void full_cycle_after_reset_matches_first_cycle() {
    Pool<> pool(sizeof(void*), 4);

    void* first[4];
    for (int i = 0; i < 4; ++i) {
        first[i] = pool.allocate();
        CHK(first[i] != nullptr);
    }
    CHK(pool.allocate() == nullptr);

    pool.reset();
    CHK(pool.usedBlocks() == 0);
    CHK(pool.freeBlocks() == 4);

    void* second[4];
    for (int i = 0; i < 4; ++i) {
        second[i] = pool.allocate();
        CHK(second[i] != nullptr);
    }
    CHK(pool.allocate() == nullptr);

    for (int i = 0; i < 4; ++i)
        CHK(second[i] == first[i]);
}

// Verifies reset() clears free-listed blocks too, not just untouched
// watermark capacity.
static void reset_after_partial_use_discards_freelist() {
    Pool<> pool(sizeof(void*), 4);
    void* a = pool.allocate();
    (void)pool.allocate();
    pool.deallocate(a);
    CHK(pool.freeBlocks() == 3); // 1 free-listed + 2 virgin

    pool.reset();
    CHK(pool.freeBlocks() == 4);
    CHK(pool.usedBlocks() == 0);

    void* reused = pool.allocate();
    CHK(reused == a); // watermark rewound to block 0, same address as `a`
}

// Verifies repeated fill/reset cycles remain stable rather than drifting
// or leaking capacity over time.
static void survives_multiple_reset_cycles() {
    Pool<> pool(sizeof(void*), 3);
    for (int cycle = 0; cycle < 5; ++cycle) {
        for (int i = 0; i < 3; ++i)
            CHK(pool.allocate() != nullptr);
        CHK(pool.allocate() == nullptr);

        pool.reset();
        CHK(pool.usedBlocks() == 0);
        CHK(pool.freeBlocks() == 3);
    }
}

// Executes all fill/reset/reuse test cases.
static void run_tests() {
    RUN(full_cycle_after_reset_matches_first_cycle);
    RUN(reset_after_partial_use_discards_freelist);
    RUN(survives_multiple_reset_cycles);
}

REGISTER_TEST_SUITE();
