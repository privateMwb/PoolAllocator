// Steady-state alloc/free cycling.
//
// Demonstrates:
// - The free-list idiom the pool is built around: allocate, use briefly,
//   free, repeat
// - After the first fill, every cycle reuses freed blocks instead of
//   growing into new memory
// - Why this pattern is the pool's actual sweet spot, versus a one-shot fill

#include <support/framework.h>

#include <cstdint>

using namespace PoolPro;

namespace {

struct Message {
    int tag;
};

} // namespace

static void run_examples() {

    setTitle("Construction");

    Pool<> pool(16, 4);
    std::cout << "totalBlocks: " << pool.totalBlocks() << "\n\n";

    // First cycle: this block is still virgin, so this is the one time the
    // watermark actually moves for it.
    setTitle("First Cycle (Watermark Moves)");

    Message* m = pool.create<Message>(1);
    auto firstAddress = reinterpret_cast<std::uintptr_t>(m);
    std::cout << "freeBlocks: " << pool.freeBlocks() << "\n";

    pool.destroy(m);
    std::cout << "freeBlocks after free: " << pool.freeBlocks() << "\n\n";

    // Every cycle after that reuses the same block off the free list — no
    // new memory is ever touched again.
    setTitle("Steady-State Cycling");

    bool allReused = true;
    for (int i = 0; i < 1000; ++i) {
        Message* cycled = pool.create<Message>(i);
        if (reinterpret_cast<std::uintptr_t>(cycled) != firstAddress)
            allReused = false;
        pool.destroy(cycled);
    }

    std::cout << "ran 1000 cycles, always reusing the same block: " << allReused << "\n";
    std::cout << "freeBlocks: " << pool.freeBlocks() << "\n";
}

REGISTER_EXAMPLE_SUITE();
