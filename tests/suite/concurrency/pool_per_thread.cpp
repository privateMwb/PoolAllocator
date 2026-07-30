// Concurrency suite: one Pool per thread.
//
// The simplest safe concurrency pattern for a type with no internal
// synchronization: never share an instance across threads at all. This
// suite confirms independently-owned pools truly don't interact —
// no shared state leaks between them, even when they're constructed,
// used, and torn down concurrently.
//
// Coverage:
// - Pools owned by different threads never hand out overlapping
//   addresses while they're simultaneously alive
// - Each thread's alloc/dealloc churn on its own pool lands on its own
//   expected final state, unaffected by every other thread's pool
// - Pools created and destroyed entirely within a thread's lifetime,
//   running concurrently across many threads, cause no interference

#include <support/framework.h>

#include <memory>
#include <thread>
#include <vector>

using namespace PoolPro;

namespace {
constexpr int kThreads = 8;
constexpr int kBlocksPerPool = 8;
} // namespace

// Verifies pools owned by different threads, alive at the same time,
// never share a backing address.
static void independent_pools_never_share_addresses() {
    std::vector<std::unique_ptr<Pool<>>> pools(kThreads);
    std::vector<std::vector<void*>> results(kThreads);
    std::vector<std::thread> threads;

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t] {
            pools[t] = std::make_unique<Pool<>>(sizeof(void*), kBlocksPerPool);
            auto& out = results[t];
            out.reserve(kBlocksPerPool);
            for (int i = 0; i < kBlocksPerPool; ++i)
                out.push_back(pools[t]->allocate());
        });
    }
    for (auto& th : threads)
        th.join();
    // Every pool is still alive here — owned by `pools`, not by the
    // lambda — so the address comparison below can't see a stale
    // address from a pool that's already been torn down.

    std::vector<void*> all;
    for (auto& v : results)
        for (void* p : v) {
            CHK(p != nullptr);
            all.push_back(p);
        }

    for (std::size_t i = 0; i < all.size(); ++i)
        for (std::size_t j = i + 1; j < all.size(); ++j)
            CHK(all[i] != all[j]);
}

// Verifies each thread's own churn on its own pool lands on the correct
// final state, regardless of what every other thread is doing.
static void per_thread_pool_state_is_fully_independent() {
    std::vector<bool> allCorrect(kThreads, true);
    std::vector<std::thread> threads;

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t] {
            Pool<> pool(sizeof(void*), kBlocksPerPool);
            // Each thread frees a different number of blocks, so a bug
            // that leaked state between pools would show up as the
            // wrong count for at least one thread.
            const int toFree = (t % kBlocksPerPool) + 1;

            void* blocks[kBlocksPerPool];
            for (int i = 0; i < kBlocksPerPool; ++i)
                blocks[i] = pool.allocate();
            for (int i = 0; i < toFree; ++i)
                pool.deallocate(blocks[i]);

            bool ok = pool.usedBlocks() == static_cast<std::size_t>(kBlocksPerPool - toFree) &&
                      pool.freeBlocks() == static_cast<std::size_t>(toFree);
            if (!ok)
                allCorrect[t] = false;
        });
    }
    for (auto& th : threads)
        th.join();

    for (bool ok : allCorrect)
        CHK(ok);
}

// Verifies pools that are created and destroyed entirely within a
// thread's own lifetime, running concurrently, don't interfere with
// each other's construction or teardown.
static void pools_created_and_destroyed_within_thread_lifetime_are_safe() {
    std::vector<bool> allCorrect(kThreads, true);
    std::vector<std::thread> threads;

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t] {
            for (int round = 0; round < 20; ++round) {
                Pool<> pool(sizeof(void*), kBlocksPerPool);
                for (int i = 0; i < kBlocksPerPool; ++i) {
                    if (pool.allocate() == nullptr)
                        allCorrect[t] = false;
                }
                // `pool` is destroyed here, every round, on every thread.
            }
        });
    }
    for (auto& th : threads)
        th.join();

    for (bool ok : allCorrect)
        CHK(ok);
}

// Executes all per-thread-pool test cases.
static void run_tests() {
    RUN(independent_pools_never_share_addresses);
    RUN(per_thread_pool_state_is_fully_independent);
    RUN(pools_created_and_destroyed_within_thread_lifetime_are_safe);
}

REGISTER_TEST_SUITE();
