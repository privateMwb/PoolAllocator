// Pool-backed objects behind std::unique_ptr.
//
// Demonstrates:
// - A custom deleter that calls Pool::destroy<T>() instead of `delete`
// - std::unique_ptr managing pool-allocated objects with normal RAII
// - The block returning to the pool automatically when the unique_ptr
//   goes out of scope

#include <support/framework.h>

#include <memory>

using namespace PoolPro;

namespace {

struct Resource {
    int id;
};

// A deleter that returns the block to `pool` instead of calling
// `operator delete`. Stores a pointer to the pool it belongs to, so it
// works with any Pool<> instance rather than a single global one.
struct PoolDeleter {
    Pool<>* pool;
    void operator()(Resource* r) const {
        pool->destroy(r);
    }
};

using ResourcePtr = std::unique_ptr<Resource, PoolDeleter>;

ResourcePtr makeResource(Pool<>& pool, int id) {
    return ResourcePtr(pool.create<Resource>(id), PoolDeleter{&pool});
}

} // namespace

static void run_examples() {

    setTitle("Construction");

    Pool<> pool(16, 8);
    std::cout << "usedBlocks: " << pool.usedBlocks() << "\n\n";

    setTitle("Owning a Pool-Allocated Object");

    {
        ResourcePtr r = makeResource(pool, 42);
        std::cout << "id: " << r->id << "\n";
        std::cout << "usedBlocks while alive: " << pool.usedBlocks() << "\n";

        // When r goes out of scope here, PoolDeleter runs destroy() — no
        // manual cleanup needed at the call site.
    }

    std::cout << "usedBlocks after scope exit: " << pool.usedBlocks() << "\n";
}

REGISTER_EXAMPLE_SUITE();
