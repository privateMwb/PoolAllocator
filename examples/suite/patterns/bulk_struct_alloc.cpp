// Creating many objects until the pool runs out.
//
// Demonstrates:
// - create<T>() in a loop, appending pointers until allocation fails
// - Detecting that the pool is exhausted, not just that the loop stopped
// - allocateBatch() as the batch-oriented alternative to the same loop

#include <support/framework.h>

#include <array>
#include <span>
#include <vector>

using namespace PoolPro;

namespace {

struct Entry {
    int id;
};

} // namespace

static void run_examples() {

    setTitle("Construction");

    // The block size is independent of sizeof(Entry) — it's just an upper
    // bound every block must fit within.
    Pool<> pool(16, 10);
    std::cout << "totalBlocks: " << pool.totalBlocks() << "\n\n";

    // create<T>() in a loop, one object at a time, stopping the instant the
    // pool can't satisfy the next request.
    setTitle("create<T>() Until Exhausted");

    std::vector<Entry*> entries;
    int nextId = 0;
    while (Entry* e = pool.create<Entry>(nextId)) {
        entries.push_back(e);
        ++nextId;
    }

    std::cout << "created  : " << entries.size() << "\n";
    std::cout << "exhausted: " << (pool.freeBlocks() == 0) << "\n\n";

    for (Entry* e : entries)
        pool.destroy(e);

    // allocateBatch() gets the same result — every block the pool has — in
    // one call instead of ten, and reports exactly how many it could
    // satisfy.
    setTitle("The Batch Alternative");

    std::array<void*, 10> raw{};
    std::size_t got = pool.allocateBatch(raw);

    std::cout << "requested: " << raw.size() << ", got: " << got << "\n";
    std::cout << "matches the loop above: " << (got == entries.size()) << "\n";

    pool.deallocateBatch(std::span<void*>(raw.data(), got));
}

REGISTER_EXAMPLE_SUITE();
