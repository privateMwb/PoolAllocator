// Allocation statistics tracking.
//
// Demonstrates:
// - Pool<true> tracking totalAllocated_, allocations_, and deallocations_
// - peakUsed_ tracking the high-water mark of simultaneous allocations
// - reset() clearing stats along with everything else, unlike an arena
//   rollback where peakUsed_ can survive

#include <support/framework.h>

using namespace PoolPro;

static void run_examples() {

    // Stats are only compiled in when EnableStats is true; Pool<> (the
    // default) has no Stats member at all.
    setTitle("Construction");

    Pool<true> pool(32, 8);
    const auto& stats = pool.getStats();

    std::cout << "totalAllocated: " << stats.totalAllocated_ << "\n";
    std::cout << "peakUsed      : " << stats.peakUsed_ << "\n\n";

    setTitle("Allocating");

    void* a = pool.allocate();
    void* b = pool.allocate();
    void* c = pool.allocate();

    std::cout << "totalAllocated: " << stats.totalAllocated_ << "\n";
    std::cout << "allocations   : " << stats.allocations_ << "\n";
    std::cout << "peakUsed      : " << stats.peakUsed_ << "\n\n";

    // Freeing blocks lowers usedBlocks(), but peakUsed_ remembers the
    // highest point it ever reached.
    setTitle("Deallocating");

    pool.deallocate(a);
    pool.deallocate(b);

    std::cout << "usedBlocks    : " << pool.usedBlocks() << "\n";
    std::cout << "deallocations : " << stats.deallocations_ << "\n";
    std::cout << "peakUsed (still the high point): " << stats.peakUsed_ << "\n\n";

    setTitle("Climbing Past the Old Peak");

    void* d = pool.allocate();
    void* e = pool.allocate();
    void* f = pool.allocate();

    std::cout << "usedBlocks: " << pool.usedBlocks() << "\n";
    std::cout << "peakUsed  : " << stats.peakUsed_ << "\n\n";

    pool.deallocate(c);
    pool.deallocate(d);
    pool.deallocate(e);
    pool.deallocate(f);

    // reset() clears both the free-list/watermark state and the stats
    // together — peakUsed_ does NOT survive a reset() here, since reset()
    // is a full return to the pool's initial state, not a partial rollback.
    setTitle("Reset Clears Stats Too");

    pool.reset();

    std::cout << "totalAllocated after reset(): " << stats.totalAllocated_ << "\n";
    std::cout << "peakUsed after reset()      : " << stats.peakUsed_ << "\n";
}

REGISTER_EXAMPLE_SUITE();
