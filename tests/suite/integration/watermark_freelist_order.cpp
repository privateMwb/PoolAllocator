// Integration suite: confirms free-listed blocks are always handed out
// before the watermark advances into virgin memory, across a long,
// interleaved sequence of operations rather than a single call pair.
//
// Coverage:
// - A scripted sequence of allocate()/deallocate() calls where each
//   result's identity is checked against the free-list-first invariant
// - Many rounds of full fill/drain churn never grow the set of distinct
//   backing addresses beyond the pool's fixed capacity
// - Single-call and batch APIs share one consistent free list

#include <support/framework.h>

using namespace PoolPro;

// Verifies, step by step, that freed blocks come back before any block
// that has never been touched.
static void freelist_preferred_over_watermark_across_sequence() {
    Pool<> pool(sizeof(void*), 8);

    void* p0 = pool.allocate();
    void* p1 = pool.allocate();
    void* p2 = pool.allocate();
    CHK(p0 != p1 && p1 != p2 && p0 != p2);

    pool.deallocate(p1);
    void* p3 = pool.allocate();
    CHK(p3 == p1); // reused, not a fresh virgin block

    void* p4 = pool.allocate();
    CHK(p4 != p0 && p4 != p1 && p4 != p2); // free list empty, must be fresh

    pool.deallocate(p0);
    pool.deallocate(p2); // free list is now LIFO: p2, then p0

    void* p5 = pool.allocate();
    CHK(p5 == p2);
    void* p6 = pool.allocate();
    CHK(p6 == p0);

    void* p7 = pool.allocate(); // free list empty again, must be fresh
    CHK(p7 != p1 && p7 != p4);
    CHK(pool.usedBlocks() == 5);
    CHK(pool.freeBlocks() == 3);
}

// Verifies many rounds of full fill/drain churn never require more
// distinct backing addresses than the pool's fixed capacity.
static void watermark_never_exceeds_capacity_under_churn() {
    Pool<> pool(sizeof(void*), 3);
    void* seen[3] = {nullptr, nullptr, nullptr};
    int seenCount = 0;

    auto remember = [&](void* p) {
        for (int i = 0; i < seenCount; ++i)
            if (seen[i] == p)
                return;
        seen[seenCount++] = p;
    };

    for (int round = 0; round < 20; ++round) {
        void* live[3];
        for (int i = 0; i < 3; ++i) {
            live[i] = pool.allocate();
            CHK(live[i] != nullptr);
            remember(live[i]);
        }
        CHK(pool.allocate() == nullptr); // exhausted every round
        CHK(seenCount <= 3);

        pool.deallocate(live[round % 3]);
        pool.deallocate(live[(round + 1) % 3]);
        void* r1 = pool.allocate();
        void* r2 = pool.allocate();
        CHK(r1 != nullptr && r2 != nullptr);
        remember(r1);
        remember(r2);
        CHK(seenCount <= 3);

        pool.deallocate(live[(round + 2) % 3]);
        pool.deallocate(r1);
        pool.deallocate(r2);
        CHK(pool.usedBlocks() == 0);
    }

    CHK(seenCount == 3); // exactly the pool's 3 backing blocks were ever used
}

// Verifies allocate()/deallocate() and their batch counterparts operate
// on one shared free list, not separate independent ones.
static void batch_and_single_calls_share_one_consistent_freelist() {
    Pool<> pool(sizeof(void*), 6);

    void* out[3];
    std::span<void*> span(out, 3);
    std::size_t got = pool.allocateBatch(span);
    CHK(got == 3);

    pool.deallocate(out[1]);
    void* single = pool.allocate();
    CHK(single == out[1]); // a plain allocate() reuses what the batch freed

    pool.deallocateBatch(span); // reclaims out[0], `single` (== out[1]), out[2]
    CHK(pool.usedBlocks() == 0);
    CHK(pool.freeBlocks() == 6);
}

// Executes all free-list/watermark ordering test cases.
static void run_tests() {
    RUN(freelist_preferred_over_watermark_across_sequence);
    RUN(watermark_never_exceeds_capacity_under_churn);
    RUN(batch_and_single_calls_share_one_consistent_freelist);
}

REGISTER_TEST_SUITE();
