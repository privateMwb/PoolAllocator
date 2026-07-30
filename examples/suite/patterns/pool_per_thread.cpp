// One pool per thread instead of a shared pool.
//
// Demonstrates:
// - Giving each thread its own Pool<> so no synchronization is needed
// - Pool having no internal locking — concurrent access to a single
//   instance from multiple threads is not supported
// - Each thread allocating, using, and freeing entirely within its own pool

#include <support/framework.h>

#include <thread>
#include <vector>

using namespace PoolPro;

namespace {

struct Job {
    int workerId;
    int result;
};

// Each thread gets its own pool — allocated, used, and torn down entirely
// within that thread, with zero contention against any other thread.
void worker(int id, int jobCount, int* resultOut) {
    Pool<> pool(sizeof(Job), jobCount);

    std::vector<Job*> jobs;
    for (int i = 0; i < jobCount; ++i)
        jobs.push_back(pool.create<Job>(id, i * i));

    int sum = 0;
    for (Job* j : jobs)
        sum += j->result;

    for (Job* j : jobs)
        pool.destroy(j);

    *resultOut = sum;
    // pool is destroyed here, at the end of the thread's own stack frame —
    // no other thread ever touches it.
}

} // namespace

static void run_examples() {

    setTitle("Launching Per-Thread Pools");

    constexpr int threadCount = 4;
    std::vector<std::thread> threads;
    std::vector<int> results(threadCount, 0);

    for (int i = 0; i < threadCount; ++i)
        threads.emplace_back(worker, i, 5, &results[i]);

    for (auto& t : threads)
        t.join();

    setTitle("Results");

    for (int i = 0; i < threadCount; ++i)
        std::cout << "thread " << i << " sum: " << results[i] << "\n";
}

REGISTER_EXAMPLE_SUITE();
