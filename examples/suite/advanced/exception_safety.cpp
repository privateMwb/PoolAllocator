// Exception safety of create<T>().
//
// Demonstrates:
// - create<T>() when T's constructor throws
// - The block being returned to the pool before the exception propagates
// - freeBlocks() left exactly as if the failed call had never happened

#include <support/framework.h>

#include <stdexcept>

using namespace PoolPro;

namespace {

struct Picky {
    int value;
    explicit Picky(int v) : value(v) {
        if (v < 0)
            throw std::invalid_argument("Picky requires a non-negative value");
    }
};

} // namespace

static void run_examples() {

    setTitle("Construction");

    Pool<> pool(16, 4);
    std::cout << "freeBlocks before: " << pool.freeBlocks() << "\n\n";

    // A successful construction behaves exactly like any other create<T>().
    setTitle("Successful Construction");

    Picky* ok = pool.create<Picky>(5);
    std::cout << "ok->value : " << ok->value << "\n";
    std::cout << "freeBlocks: " << pool.freeBlocks() << "\n\n";
    pool.destroy(ok);

    // When Picky's constructor throws, create<T>() catches the exception,
    // hands the block back to the pool, and rethrows — the caller never
    // sees a half-constructed object, and no block is lost.
    setTitle("Constructor Throws");

    std::size_t freeBefore = pool.freeBlocks();
    try {
        Picky* bad = pool.create<Picky>(-1);
        (void)bad; // unreachable
    } catch (const std::invalid_argument& e) {
        std::cout << "caught: " << e.what() << "\n";
    }

    std::cout << "freeBlocks unchanged: " << (pool.freeBlocks() == freeBefore) << "\n";
    std::cout << "freeBlocks: " << pool.freeBlocks() << "\n";
}

REGISTER_EXAMPLE_SUITE();
