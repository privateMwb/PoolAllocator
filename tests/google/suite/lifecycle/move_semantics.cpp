// Pool move-semantics test suite.
//
// Coverage:
// - Move construction transfers full state (block count, used/free
//   counts, ownership of already-allocated pointers)
// - The moved-from pool is left valid and zero-capacity
// - Move assignment releases the target's existing allocation before
//   taking ownership of the source's
// - Self-move-assignment is safe and leaves the pool unchanged

#include <gtest/gtest.h>

#include <PoolPro/Pool.h>

#include <utility>

using namespace PoolPro;

// Verifies move construction carries over block counts and ownership of
// pointers already handed out by the source.
TEST(MoveSemantics, MoveConstructTransfersState) {
    Pool<> source(sizeof(void*), 4);
    void* p = source.allocate();
    (void)source.allocate();

    Pool<> target(std::move(source));
    EXPECT_EQ(target.totalBlocks(), 4u);
    EXPECT_EQ(target.usedBlocks(), 2u);
    EXPECT_EQ(target.freeBlocks(), 2u);
    EXPECT_TRUE(target.owns(p));
}

// Verifies the moved-from pool is left valid but empty, and that using
// it afterward (e.g. allocate()) fails harmlessly rather than crashing.
TEST(MoveSemantics, MoveConstructLeavesSourceEmpty) {
    Pool<> source(sizeof(void*), 4);
    (void)source.allocate();

    Pool<> target(std::move(source));
    (void)target;

    // NOLINTNEXTLINE(clang-analyzer-cplusplus.Move)
    EXPECT_EQ(source.totalBlocks(), 0u);
    EXPECT_EQ(source.capacity(), 0u);
    EXPECT_EQ(source.allocate(), nullptr);
}

// Verifies move assignment tears down the target's own backing
// allocation before taking ownership of the source's.
TEST(MoveSemantics, MoveAssignReplacesExistingAllocation) {
    Pool<> target(sizeof(void*), 2);
    void* oldPtr = target.allocate();

    Pool<> source(sizeof(void*), 5);
    target = std::move(source);

    EXPECT_EQ(target.totalBlocks(), 5u);
    EXPECT_FALSE(target.owns(oldPtr));
}

namespace {

// Indirection so the compiler can't flag this as an obvious self-move
// at the call site; exercises the same code path a generic algorithm
// (e.g. std::swap-based reassignment) could trigger.
void selfMoveAssign(Pool<>& pool) {
    pool = std::move(pool);
}

} // namespace

// Verifies self-move-assignment neither corrupts state nor crashes.
TEST(MoveSemantics, SelfMoveAssignmentIsSafe) {
    Pool<> pool(sizeof(void*), 3);
    void* p = pool.allocate();

    selfMoveAssign(pool);

    EXPECT_EQ(pool.totalBlocks(), 3u);
    EXPECT_EQ(pool.usedBlocks(), 1u);
    EXPECT_TRUE(pool.owns(p));
}
