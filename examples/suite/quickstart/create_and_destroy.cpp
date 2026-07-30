// Object lifetime across several create<T>() calls.
//
// Demonstrates:
// - Constructing several objects with create<T>()
// - How destroy() differs from an arena's: the block is returned right away
// - usedBlocks()/freeBlocks() tracking lifetime precisely, not just growing

#include <support/framework.h>

#include <cstdint>

using namespace PoolPro;

namespace {

struct Widget {
    int id;
    double value;
};

} // namespace

static void run_examples() {

    setTitle("Construction");

    Pool<> pool(sizeof(Widget), 4);
    std::cout << "totalBlocks: " << pool.totalBlocks() << "\n\n";

    setTitle("Several create<T>() Calls");

    Widget* a = pool.create<Widget>(1, 1.5);
    Widget* b = pool.create<Widget>(2, 2.5);
    Widget* c = pool.create<Widget>(3, 3.5);
    auto bAddress = reinterpret_cast<std::uintptr_t>(b);

    std::cout << "a: id=" << a->id << " value=" << a->value << "\n";
    std::cout << "b: id=" << b->id << " value=" << b->value << "\n";
    std::cout << "c: id=" << c->id << " value=" << c->value << "\n";
    std::cout << "usedBlocks: " << pool.usedBlocks() << "\n\n";

    // Unlike an arena, destroy() here returns the block to the free list
    // immediately — usedBlocks() drops right away instead of only shrinking
    // on the next reset().
    setTitle("Destroying One Object");

    pool.destroy(b);
    std::cout << "usedBlocks after destroy(b): " << pool.usedBlocks() << "\n";
    std::cout << "freeBlocks after destroy(b): " << pool.freeBlocks() << "\n\n";

    // The freed block is immediately available again — this create<T>()
    // reuses b's old block rather than bumping the watermark.
    setTitle("Reusing a Freed Block");

    Widget* d = pool.create<Widget>(4, 4.5);
    auto dAddress = reinterpret_cast<std::uintptr_t>(d);

    std::cout << "d: id=" << d->id << " value=" << d->value << "\n";
    std::cout << "d reused b's block: " << (dAddress == bAddress) << "\n";
    std::cout << "usedBlocks: " << pool.usedBlocks() << "\n\n";

    setTitle("Cleanup");

    pool.destroy(a);
    pool.destroy(c);
    pool.destroy(d);
    std::cout << "usedBlocks after cleanup: " << pool.usedBlocks() << "\n";
}

REGISTER_EXAMPLE_SUITE();
