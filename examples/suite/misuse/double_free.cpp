// Freeing the same block twice.
//
// Demonstrates:
// - The immediate double-free pattern AP_ASSERT(ptr != freeList_) catches
// - Why it only catches THIS pattern, not a double-free separated by other
//   calls
// - Shown but not executed — this would trip an assert in debug builds and
//   corrupt the free list in release builds

#include <support/framework.h>

using namespace PoolPro;

namespace {

struct Token {
    int value;
};

} // namespace

static void run_examples() {

    setTitle("Construction");

    Pool<> pool(16, 4);
    Token* t = pool.create<Token>(1);

    std::cout << "usedBlocks: " << pool.usedBlocks() << "\n\n";

    // The mistake: freeing the same pointer twice in a row. In a debug
    // build this trips AP_ASSERT(ptr != freeList_) and aborts; in a
    // release build (NDEBUG) the assert compiles out, and the second
    // deallocate() links the block onto the free list a second time,
    // corrupting it — a later allocate() can then hand out the same
    // memory to two different callers simultaneously.
    //
    // Not executed here on purpose:
    //
    //   pool.destroy(t);
    //   pool.deallocate(t);   // <-- immediate double-free, caught by
    //                         //     AP_ASSERT in debug, corrupts the
    //                         //     free list in release
    setTitle("The Mistake (Not Executed)");

    std::cout << "see the comment above — freeing `t` a second time here\n"
                 "would double-link the same block onto the free list\n\n";

    // What AP_ASSERT does NOT catch: a double-free separated by other pool
    // activity. By the time the second deallocate(t) runs, ptr is no
    // longer freeList_'s head, so the cheap check passes right through —
    // this pattern corrupts the free list silently, even in debug builds.
    setTitle("What The Check Misses (Not Executed)");

    std::cout << "pool.destroy(t); pool.allocate(); pool.deallocate(t); /* second\n"
                 "free, but t != freeList_ anymore, so the assert doesn't fire */\n\n";

    setTitle("Cleanup");

    pool.destroy(t);
    std::cout << "usedBlocks after proper destroy(): " << pool.usedBlocks() << "\n";
}

REGISTER_EXAMPLE_SUITE();
