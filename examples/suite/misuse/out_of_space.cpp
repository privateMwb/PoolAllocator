// Running out of blocks.
//
// Demonstrates:
// - allocate() returning nullptr instead of throwing once the pool is
//   exhausted
// - Why every caller of allocate() must check the result before using it
// - create<T>() propagating the same nullptr instead of throwing bad_alloc

#include <support/framework.h>

using namespace PoolPro;

namespace {

struct Tag {
    int value;
};

} // namespace

static void run_examples() {

    setTitle("Construction");

    Pool<> pool(32, 2);
    std::cout << "totalBlocks: " << pool.totalBlocks() << "\n\n";

    setTitle("Exhausting the Pool");

    void* a = pool.allocate();
    void* b = pool.allocate();
    std::cout << "a: " << (a != nullptr) << ", b: " << (b != nullptr) << "\n";
    std::cout << "freeBlocks: " << pool.freeBlocks() << "\n\n";

    // The mistake: calling allocate() without checking the pool has room,
    // then using the result as if it were a valid pointer. A pool never
    // throws to signal exhaustion — it returns nullptr, silently, just
    // like malloc().
    setTitle("The Mistake: Unchecked allocate()");

    void* c = pool.allocate();
    std::cout << "c == nullptr: " << (c == nullptr) << "\n";
    // Writing through `c` here without checking it first would be a null
    // pointer dereference — not shown, since it's undefined behavior.

    setTitle("The Fix: Check Before Use");

    if (c == nullptr) {
        std::cout << "pool exhausted — handle gracefully instead of dereferencing\n";
    }

    // create<T>() has the same contract: if the pool is exhausted, it
    // returns nullptr rather than constructing anything or throwing.
    setTitle("create<T>() Under Exhaustion");

    Tag* t = pool.create<Tag>(1);
    std::cout << "create<T>() also returned nullptr: " << (t == nullptr) << "\n";
}

REGISTER_EXAMPLE_SUITE();
