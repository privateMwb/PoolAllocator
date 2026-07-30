// Move semantics.
//
// Demonstrates:
// - Move-constructing a pool
// - Move-assigning a pool
// - What's left behind in a moved-from pool, and what's safe to call on it

#include <support/framework.h>

using namespace PoolPro;

static void run_examples() {

    setTitle("Move Construction");

    Pool<> original(64, 8);
    void* block = original.allocate();

    std::cout << "original usedBlocks before move: " << original.usedBlocks() << "\n";

    Pool<> moved(std::move(original));

    std::cout << "moved usedBlocks : " << moved.usedBlocks() << "\n";
    std::cout << "moved totalBlocks: " << moved.totalBlocks() << "\n";
    std::cout << "moved owns(block): " << moved.owns(block) << "\n\n";

    // A moved-from pool is left in a valid, empty (zero-capacity) state —
    // not destroyed, just holding nothing.
    setTitle("Moved-From State");

    std::cout << "original totalBlocks: " << original.totalBlocks() << "\n";
    std::cout << "original capacity   : " << original.capacity() << "\n";
    std::cout << "original owns(block): " << original.owns(block) << "\n\n";

    // Calling allocate() on a moved-from pool is safe — it simply has zero
    // capacity, so it returns nullptr rather than doing anything undefined.
    setTitle("Safe Calls on a Moved-From Pool");

    void* fromEmpty = original.allocate();
    std::cout << "allocate() on moved-from pool is nullptr: " << (fromEmpty == nullptr) << "\n\n";

    setTitle("Move Assignment");

    Pool<> a(32, 4);
    Pool<> b(64, 2);
    void* aBlock = a.allocate();

    std::cout << "a totalBlocks before assign: " << a.totalBlocks() << "\n";
    std::cout << "b totalBlocks before assign: " << b.totalBlocks() << "\n";

    // Move-assignment releases b's own backing allocation first, then
    // takes over a's.
    b = std::move(a);

    std::cout << "b totalBlocks after assign : " << b.totalBlocks() << "\n";
    std::cout << "b owns(aBlock) after assign: " << b.owns(aBlock) << "\n";
    std::cout << "a totalBlocks after assign : " << a.totalBlocks() << " (moved-from)\n";
}

REGISTER_EXAMPLE_SUITE();
