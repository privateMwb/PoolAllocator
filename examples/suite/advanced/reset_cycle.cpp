// Fill / drain / reset cycling.
//
// Demonstrates:
// - The watermark only ever moving forward through virgin memory
// - A full fill-then-drain cycle exhausting the free list without ever
//   revisiting the watermark
// - reset() as an O(1) return to the initial state, ready for another cycle

#include <support/framework.h>

#include <array>

using namespace PoolPro;

static void run_examples() {

    setTitle("Construction");

    Pool<> pool(32, 4);
    std::cout << "totalBlocks: " << pool.totalBlocks() << "\n\n";

    // Filling the pool for the first time bumps the watermark through
    // every block — this is the only time these blocks are ever "touched"
    // by allocate().
    setTitle("First Fill");

    std::array<void*, 4> blocks{};
    for (auto& b : blocks)
        b = pool.allocate();

    std::cout << "freeBlocks after first fill: " << pool.freeBlocks() << "\n\n";

    // Draining — freeing every block — threads them onto the free list.
    // A subsequent fill reuses that list; it never touches the watermark
    // again, because the watermark already reached the end.
    setTitle("Drain and Refill");

    for (void* b : blocks)
        pool.deallocate(b);

    std::cout << "freeBlocks after drain: " << pool.freeBlocks() << "\n";

    for (auto& b : blocks)
        b = pool.allocate();

    std::cout << "freeBlocks after refill: " << pool.freeBlocks() << "\n\n";
    std::cout << "a pool cycled like this runs indefinitely at O(1) per\n"
                 "allocate()/deallocate(), with no per-block cost paid twice\n\n";

    // reset() is a separate, explicit return to the initial state —
    // clearing the free list and rewinding the watermark to 0 in one O(1)
    // step, regardless of how many blocks are outstanding.
    setTitle("Explicit Reset");

    pool.reset();

    std::cout << "freeBlocks after reset(): " << pool.freeBlocks() << "\n";
    std::cout << "usedBlocks after reset() : " << pool.usedBlocks() << "\n";
}

REGISTER_EXAMPLE_SUITE();
