// Basic Pool usage.
//
// Demonstrates:
// - Constructing a pool with a fixed block size and block count
// - Raw allocation with allocate() and deallocate()
// - Constructing an object in place with create()
// - Destroying an object with destroy()
// - Capacity, used, and free block checks
// - Resetting a pool

#include <support/framework.h>

using namespace PoolPro;

namespace {

struct Point {
    int x;
    int y;
};

} // namespace

static void run_examples() {

    // A pool is constructed with a block size and a block count — every
    // block is at least blockSize bytes, and the pool holds blockCount of
    // them, up front, in one allocation.
    setTitle("Construction");

    Pool<> pool(64, 16);

    std::cout << "capacity    : " << pool.capacity() << "\n";
    std::cout << "totalBlocks : " << pool.totalBlocks() << "\n";
    std::cout << "usedBlocks  : " << pool.usedBlocks() << "\n";
    std::cout << "freeBlocks  : " << pool.freeBlocks() << "\n\n";

    // allocate() hands back one raw, uninitialized block, or nullptr if
    // the pool is exhausted.
    setTitle("Raw Allocation");

    void* block = pool.allocate();
    std::cout << "allocate() succeeded: " << (block != nullptr) << "\n";
    std::cout << "freeBlocks after allocate: " << pool.freeBlocks() << "\n\n";

    // deallocate() returns a block to the pool without touching its
    // contents — unlike an arena, a pool can free a single block at a time.
    setTitle("Raw Deallocation");

    pool.deallocate(block);
    std::cout << "freeBlocks after deallocate: " << pool.freeBlocks() << "\n\n";

    // create<T>() allocates a block and constructs T in place, returning a
    // ready-to-use pointer.
    setTitle("Construction In Place");

    Point* p = pool.create<Point>(3, 4);
    std::cout << "point: (" << p->x << ", " << p->y << ")\n\n";

    // destroy() runs the object's destructor and returns its block to the
    // pool in one call.
    setTitle("Destruction");

    pool.destroy(p);
    std::cout << "freeBlocks after destroy(): " << pool.freeBlocks() << "\n\n";

    // usedBlocks() and freeBlocks() move together; capacity() and
    // totalBlocks() never change.
    setTitle("Capacity and Usage");

    std::cout << "capacity    : " << pool.capacity() << "\n";
    std::cout << "totalBlocks : " << pool.totalBlocks() << "\n";
    std::cout << "usedBlocks  : " << pool.usedBlocks() << "\n";
    std::cout << "freeBlocks  : " << pool.freeBlocks() << "\n\n";

    // reset() restores the pool to its initial state in O(1), making every
    // block available again. It does not touch or zero the backing memory.
    setTitle("Reset");

    pool.reset();

    std::cout << "usedBlocks after reset(): " << pool.usedBlocks() << "\n";
    std::cout << "freeBlocks after reset(): " << pool.freeBlocks() << "\n";
}

REGISTER_EXAMPLE_SUITE();
