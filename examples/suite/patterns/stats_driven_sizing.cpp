// Sizing a pool from observed peak usage.
//
// Demonstrates:
// - Running a representative workload against Pool<true>
// - Reading peakUsed_ back out via getStats()
// - Sizing a production Pool<> from that number instead of guessing

#include <support/framework.h>

#include <array>

using namespace PoolPro;

namespace {

// A stand-in for some representative workload: a bursty pattern of
// allocations and frees that never holds more than a handful of blocks at
// once.
std::size_t runWorkload(Pool<true>& pool) {
    std::array<void*, 6> held{};
    std::size_t heldCount = 0;

    for (int round = 0; round < 3; ++round) {
        while (heldCount < held.size()) {
            void* b = pool.allocate();
            if (!b)
                break;
            held[heldCount++] = b;
        }
        while (heldCount > 2) {
            pool.deallocate(held[--heldCount]);
        }
    }
    while (heldCount > 0) {
        pool.deallocate(held[--heldCount]);
    }

    return pool.getStats().peakUsed_;
}

} // namespace

static void run_examples() {

    // Measure against an oversized pool first — the point is to observe
    // real usage, not to constrain it.
    setTitle("Measuring a Representative Workload");

    Pool<true> measuring(32, 64);
    std::size_t peak = runWorkload(measuring);

    std::cout << "peakUsed observed: " << peak << "\n";
    std::cout << "totalAllocated   : " << measuring.getStats().totalAllocated_ << "\n\n";

    // Size the production pool from the observed peak, with a small margin
    // instead of the oversized capacity used for measurement.
    setTitle("Sizing The Production Pool");

    std::size_t productionCapacity = peak + peak / 4; // 25% headroom
    Pool<> production(32, productionCapacity);

    std::cout << "production totalBlocks: " << production.totalBlocks() << "\n";
    std::cout << "(vs. 64 blocks guessed without measurement)\n";
}

REGISTER_EXAMPLE_SUITE();
