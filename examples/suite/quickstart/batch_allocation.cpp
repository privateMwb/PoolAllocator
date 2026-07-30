// Batch allocation and deallocation.
//
// Demonstrates:
// - allocateBatch() filling multiple pointers in one call
// - A batch request larger than what remains, and the partial count returned
// - deallocateBatch() returning many blocks in one call

#include <support/framework.h>

#include <array>
#include <span>

using namespace PoolPro;

static void run_examples() {

    setTitle("Construction");

    Pool<> pool(32, 8);
    std::cout << "totalBlocks: " << pool.totalBlocks() << "\n\n";

    // allocateBatch() drains the free list first, then bump-allocates the
    // rest from virgin memory — all in one call instead of one allocate()
    // per block.
    setTitle("Allocating a Batch");

    std::array<void*, 5> batch{};
    std::size_t got = pool.allocateBatch(batch);

    std::cout << "requested: 5, got: " << got << "\n";
    std::cout << "freeBlocks after batch: " << pool.freeBlocks() << "\n\n";

    // Asking for more than the pool has left returns fewer blocks than
    // requested instead of failing outright — only the first `got` entries
    // of `out` are valid.
    setTitle("Requesting More Than Remains");

    std::array<void*, 5> overflow{};
    std::size_t got2 = pool.allocateBatch(overflow);

    std::cout << "requested: 5, got: " << got2 << " (pool only had " << (pool.totalBlocks() - got)
              << " blocks left)\n";
    std::cout << "freeBlocks after second batch: " << pool.freeBlocks() << "\n\n";

    // deallocateBatch() writes freeList_ back once for the whole span,
    // instead of once per pointer.
    setTitle("Deallocating a Batch");

    pool.deallocateBatch(std::span<void*>(batch.data(), got));
    std::cout << "freeBlocks after deallocateBatch: " << pool.freeBlocks() << "\n";

    pool.deallocateBatch(std::span<void*>(overflow.data(), got2));
    std::cout << "freeBlocks after second deallocateBatch: " << pool.freeBlocks() << "\n";
}

REGISTER_EXAMPLE_SUITE();
