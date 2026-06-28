// Example Basic
//
// Covers:
// - Pool construction.
// - Raw memory allocation.
// - Batch memory allocation.
// - Pool reset.
// - Pool introspection.

#include "example_helper.h"

using namespace AllocatorPro;

int main() {
    mainTitle("\nBasic Examples");
    borderLine();

    // Construction
    setTitle("Construction");

    Pool pool{64, 8};

    std::cout << "Block size  : " << pool.blockStride()  << "\n";
    std::cout << "Total blocks: " << pool.totalBlocks()  << "\n";
    std::cout << "Free blocks : " << pool.freeBlocks()   << "\n";
    std::cout << "Used blocks : " << pool.usedBlocks()   << "\n";
    std::cout << "Capacity    : " << pool.capacity()     << "\n\n";

    // Raw Allocation
    setTitle("Raw Allocation");

    void* p1 = pool.allocate();
    void* p2 = pool.allocate();
    void* p3 = pool.allocate();
    (void)p1;
    (void)p2;
    (void)p3;

    std::cout << "Used after 3 allocations : " << pool.usedBlocks() << "\n";
    std::cout << "Free after 3 allocations : " << pool.freeBlocks() << "\n\n";

    // Deallocation
    setTitle("Deallocation");

    pool.deallocate(p1);
    pool.deallocate(p2);

    std::cout << "Used after 2 deallocations : " << pool.usedBlocks() << "\n";
    std::cout << "Free after 2 deallocations : " << pool.freeBlocks() << "\n\n";

    // Batch Allocation
    setTitle("Batch Allocation");

    pool.reset();
    void* blocks[8]{};
    std::size_t count = pool.allocateBatch(blocks);

    std::cout << "Blocks allocated : " << count             << "\n";
    std::cout << "Used blocks      : " << pool.usedBlocks() << "\n";
    std::cout << "Free blocks      : " << pool.freeBlocks() << "\n\n";

    // Introspection
    setTitle("Introspection");

    std::cout << "Owns blocks[0] : " << pool.owns(blocks[0]) << "\n";
    std::cout << "Owns blocks[7] : " << pool.owns(blocks[7]) << "\n";

    int x = 0;
    std::cout << "Owns &x        : " << pool.owns(&x)        << "\n\n";

    // Reset
    setTitle("Reset");

    std::cout << "Used before reset : " << pool.usedBlocks() << "\n";
    pool.reset();
    std::cout << "Used after reset  : " << pool.usedBlocks() << "\n";
    std::cout << "Free after reset  : " << pool.freeBlocks() << "\n";

    borderLine();
    std::cout << "\n";
    return 0;
}


