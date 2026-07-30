// A pointer left dangling by reset().
//
// Demonstrates:
// - reset() clearing the free list and watermark without destroying or
//   even touching any block's contents
// - A pointer obtained before reset() aliasing a completely different
//   logical allocation afterward
// - Why callers must destroy() everything they care about before reset()

#include <support/framework.h>

#include <cstdint>

using namespace PoolPro;

namespace {

struct Handle {
    int id;
};

} // namespace

static void run_examples() {

    setTitle("Construction");

    Pool<> pool(16, 4);

    setTitle("Allocating Before Reset");

    Handle* h = pool.create<Handle>(7);
    auto hAddress = reinterpret_cast<std::uintptr_t>(h);
    std::cout << "h->id: " << h->id << "\n";
    std::cout << "usedBlocks: " << pool.usedBlocks() << "\n\n";

    // The mistake: reset() does not know or care that `h` still points at
    // a live Handle. It just rewinds the pool to empty — `h` is now a
    // dangling pointer, even though the memory it points to hasn't
    // physically changed yet.
    setTitle("The Mistake: reset() Without destroy()");

    pool.reset();

    std::cout << "usedBlocks after reset(): " << pool.usedBlocks() << "\n";
    std::cout << "h is now dangling — do not dereference it\n\n";

    // The pool is free to hand that exact address back out to the very
    // next caller, for a completely unrelated object.
    setTitle("The Same Address, Reused");

    Handle* h2 = pool.create<Handle>(99);
    auto h2Address = reinterpret_cast<std::uintptr_t>(h2);

    std::cout << "h2->id           : " << h2->id << "\n";
    std::cout << "same address as h: " << (hAddress == h2Address) << "\n";
    std::cout << "dereferencing the original h here would silently read h2's\n"
                 "data instead — the fix is to destroy() every live object\n"
                 "before calling reset()\n";

    pool.destroy(h2);
}

REGISTER_EXAMPLE_SUITE();
