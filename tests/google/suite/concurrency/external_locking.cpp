// Concurrency suite: external locking.
//
// Pool has no internal synchronization of its own (see Pool.h/Pool.tpp —
// no mutex, no atomics); it's the caller's job to serialize access from
// multiple threads. This suite confirms that a single external mutex
// wrapped around every call is sufficient for correct, race-free use
// from multiple threads.
//
// Coverage:
// - Concurrent allocate() calls, serialized by a shared mutex, never
//   hand the same block out twice
// - Concurrent deallocate() calls reclaim exactly what was allocated,
//   with no blocks lost or double-counted
// - Concurrent allocateBatch() calls under the same lock stay consistent

#include <gtest/gtest.h>

#include <PoolPro/Pool.h>

#include <array>
#include <cstddef>
#include <mutex>
#include <span>
#include <thread>
#include <vector>

using namespace PoolPro;

namespace {
constexpr int kThreads = 8;
constexpr int kPerThread = 50;
} // namespace

// Verifies no two threads are ever handed the same block.
TEST(ExternalLocking, ConcurrentAllocNoDoubleIssue) {
    Pool<> pool(sizeof(void*), kThreads * kPerThread);
    std::mutex poolMutex;

    std::vector<std::vector<void*>> results(kThreads);
    std::vector<std::thread> threads;

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t] {
            auto& out = results[t];
            out.reserve(kPerThread);
            for (int i = 0; i < kPerThread; ++i) {
                std::lock_guard<std::mutex> lock(poolMutex);
                void* p = pool.allocate();
                out.push_back(p);
            }
        });
    }
    for (auto& th : threads)
        th.join();

    std::vector<void*> all;
    all.reserve(kThreads * kPerThread);
    for (auto& v : results)
        for (void* p : v) {
            EXPECT_NE(p, nullptr);
            all.push_back(p);
        }

    for (std::size_t i = 0; i < all.size(); ++i)
        for (std::size_t j = i + 1; j < all.size(); ++j)
            EXPECT_NE(all[i], all[j]);

    EXPECT_EQ(pool.usedBlocks(), static_cast<std::size_t>(kThreads * kPerThread));
    EXPECT_EQ(pool.freeBlocks(), 0u);
}

// Verifies concurrent deallocate() calls reclaim exactly the blocks that
// were handed out, leaving the pool fully available again.
TEST(ExternalLocking, ConcurrentDeallocReclaimsExact) {
    Pool<> pool(sizeof(void*), kThreads * kPerThread);
    std::mutex poolMutex;

    std::vector<std::vector<void*>> owned(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        owned[t].reserve(kPerThread);
        for (int i = 0; i < kPerThread; ++i)
            owned[t].push_back(pool.allocate());
    }
    EXPECT_EQ(pool.usedBlocks(), static_cast<std::size_t>(kThreads * kPerThread));

    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t] {
            for (void* p : owned[t]) {
                std::lock_guard<std::mutex> lock(poolMutex);
                pool.deallocate(p);
            }
        });
    }
    for (auto& th : threads)
        th.join();

    EXPECT_EQ(pool.usedBlocks(), 0u);
    EXPECT_EQ(pool.freeBlocks(), static_cast<std::size_t>(kThreads * kPerThread));
}

// Verifies allocateBatch(), called concurrently under the same external
// lock, still hands out exactly one distinct block per slot with no
// duplicates or lost blocks.
TEST(ExternalLocking, ConcurrentAllocBatchConsistent) {
    constexpr int kBatchSize = 5;
    Pool<> pool(sizeof(void*), kThreads * kBatchSize);
    std::mutex poolMutex;

    std::vector<std::array<void*, kBatchSize>> results(kThreads);
    std::vector<std::size_t> counts(kThreads, 0);
    std::vector<std::thread> threads;

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t] {
            std::span<void*> span(results[t].data(), kBatchSize);
            std::lock_guard<std::mutex> lock(poolMutex);
            counts[t] = pool.allocateBatch(span);
        });
    }
    for (auto& th : threads)
        th.join();

    // Checked here rather than inside the worker threads.
    for (int t = 0; t < kThreads; ++t)
        EXPECT_EQ(counts[t], static_cast<std::size_t>(kBatchSize)) << "thread " << t;

    std::vector<void*> all;
    for (auto& arr : results)
        for (void* p : arr)
            all.push_back(p);

    for (std::size_t i = 0; i < all.size(); ++i)
        for (std::size_t j = i + 1; j < all.size(); ++j)
            EXPECT_NE(all[i], all[j]);

    EXPECT_EQ(pool.usedBlocks(), static_cast<std::size_t>(kThreads * kBatchSize));
}
