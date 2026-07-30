// destroy()'s owns(ptr) precondition.
//
// Demonstrates:
// - destroy() silently ignoring a pointer that isn't owned by the pool,
//   instead of corrupting internal state
// - Why that safety net doesn't excuse passing garbage pointers on purpose
// - The same behavior guarding deallocate()

#include <support/framework.h>

using namespace PoolPro;

namespace {

struct Token {
    int value;
};

} // namespace

static void run_examples() {

    setTitle("Construction");

    Pool<> poolA(16, 4);
    Pool<> poolB(16, 4);

    Token* fromA = poolA.create<Token>(1);
    Token* fromB = poolB.create<Token>(2);

    std::cout << "poolA usedBlocks: " << poolA.usedBlocks() << "\n";
    std::cout << "poolB usedBlocks: " << poolB.usedBlocks() << "\n\n";

    // The mistake: passing a pointer that belongs to a different pool to
    // destroy(). owns() catches this and destroy() becomes a no-op — it
    // does NOT run ~Token() and does NOT touch poolA's free list.
    setTitle("The Mistake: Cross-Pool destroy()");

    poolA.destroy(fromB);

    std::cout << "poolA usedBlocks unchanged  : " << poolA.usedBlocks() << "\n";
    std::cout << "poolB usedBlocks unchanged  : " << poolB.usedBlocks() << "\n";
    std::cout << "fromB->value still readable : " << fromB->value << "\n\n";

    // A stack object triggers the same guard.
    setTitle("A Non-Pool Pointer");

    Token onStack{3};
    poolA.destroy(&onStack);
    std::cout << "poolA usedBlocks still unchanged: " << poolA.usedBlocks() << "\n\n";

    // This is a safety net, not a license to pass arbitrary pointers on
    // purpose — relying on it to "detect" ownership is itself a misuse of
    // the API; the check exists to make accidental misuse survivable, not
    // to be part of normal control flow.
    setTitle("Cleanup");

    poolA.destroy(fromA);
    poolB.destroy(fromB);
}

REGISTER_EXAMPLE_SUITE();
