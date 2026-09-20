// Concurrency suite: concurrent read-only observers.
//
// usedBlocks(), freeBlocks(), totalBlocks(), capacity(), owns(), and
// getStats() only read state; they're documented AP_PURE where
// applicable. Once a pool's state has settled (no concurrent writers),
// multiple threads reading it at the same time is safe and must return
// identical, non-torn values.
//
// Coverage:
// - Concurrent introspection reads (usedBlocks/freeBlocks/totalBlocks/
//   capacity) all agree with the expected fixed values
// - Concurrent owns() checks agree across threads for both owned and
//   foreign pointers
// - Concurrent getStats() reads agree on every field

#include <support/framework.h>

#include <thread>
#include <vector>

using namespace PoolPro;

namespace {
constexpr int kThreads = 8;
constexpr int kReadsPerThread = 200;
} // namespace

// Verifies concurrent reads of the block-count accessors all agree with
// the pool's actual, unchanging state.
static void concurrent_introspection_reads_agree() {
    Pool<> pool(sizeof(void*), 10);
    for (int i = 0; i < 6; ++i)
        (void)pool.allocate();

    std::vector<bool> allCorrect(kThreads, true);
    std::vector<std::thread> threads;

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t] {
            for (int i = 0; i < kReadsPerThread; ++i) {
                bool ok = pool.usedBlocks() == 6 && pool.freeBlocks() == 4 &&
                          pool.totalBlocks() == 10 && pool.capacity() == pool.blockStride() * 10;
                if (!ok)
                    allCorrect[t] = false;
            }
        });
    }
    for (auto& th : threads)
        th.join();

    for (bool ok : allCorrect)
        CHK(ok);
}

// Verifies concurrent owns() checks agree across threads for a mix of
// owned and foreign pointers.
static void concurrent_owns_checks_agree() {
    Pool<> pool(sizeof(void*), 4);
    void* owned = pool.allocate();
    int stackVar = 0;

    std::vector<bool> allCorrect(kThreads, true);
    std::vector<std::thread> threads;

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t] {
            for (int i = 0; i < kReadsPerThread; ++i) {
                bool ok = pool.owns(owned) && !pool.owns(&stackVar) && !pool.owns(nullptr);
                if (!ok)
                    allCorrect[t] = false;
            }
        });
    }
    for (auto& th : threads)
        th.join();

    for (bool ok : allCorrect)
        CHK(ok);
}

// Verifies concurrent getStats() reads agree on every field once the
// pool's stats have settled.
static void concurrent_stats_reads_agree() {
    Pool<true> pool(sizeof(void*), 5);
    void* a = pool.allocate();
    (void)pool.allocate();
    pool.deallocate(a);

    std::vector<bool> allCorrect(kThreads, true);
    std::vector<std::thread> threads;

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t] {
            for (int i = 0; i < kReadsPerThread; ++i) {
                const auto& stats = pool.getStats();
                bool ok = stats.totalAllocated_ == 2 && stats.allocations_ == 2 &&
                          stats.deallocations_ == 1 && stats.peakUsed_ == 2;
                if (!ok)
                    allCorrect[t] = false;
            }
        });
    }
    for (auto& th : threads)
        th.join();

    for (bool ok : allCorrect)
        CHK(ok);
}

// Executes all concurrent-observer test cases.
static void run_tests() {
    RUN(concurrent_introspection_reads_agree);
    RUN(concurrent_owns_checks_agree);
    RUN(concurrent_stats_reads_agree);
}

REGISTER_TEST_SUITE();
