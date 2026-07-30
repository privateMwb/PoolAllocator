// Pool move-semantics test suite.
//
// Coverage:
// - Move construction transfers full state (block count, used/free
//   counts, ownership of already-allocated pointers)
// - The moved-from pool is left valid and zero-capacity
// - Move assignment releases the target's existing allocation before
//   taking ownership of the source's
// - Self-move-assignment is safe and leaves the pool unchanged

#include <support/framework.h>

#include <utility>

using namespace PoolPro;

// Verifies move construction carries over block counts and ownership of
// pointers already handed out by the source.
static void move_construct_transfers_state() {
    Pool<> source(sizeof(void*), 4);
    void* p = source.allocate();
    (void)source.allocate();

    Pool<> target(std::move(source));
    CHK(target.totalBlocks() == 4);
    CHK(target.usedBlocks() == 2);
    CHK(target.freeBlocks() == 2);
    CHK(target.owns(p));
}

// Verifies the moved-from pool is left valid but empty, and that using
// it afterward (e.g. allocate()) fails harmlessly rather than crashing.
static void move_construct_leaves_source_empty() {
    Pool<> source(sizeof(void*), 4);
    (void)source.allocate();

    Pool<> target(std::move(source));
    (void)target;

    // NOLINTNEXTLINE(clang-analyzer-cplusplus.Move)
    CHK(source.totalBlocks() == 0);
    CHK(source.capacity() == 0);
    CHK(source.allocate() == nullptr);
}

// Verifies move assignment tears down the target's own backing
// allocation before taking ownership of the source's.
static void move_assign_replaces_existing_allocation() {
    Pool<> target(sizeof(void*), 2);
    void* oldPtr = target.allocate();

    Pool<> source(sizeof(void*), 5);
    target = std::move(source);

    CHK(target.totalBlocks() == 5);
    CHK(!target.owns(oldPtr));
}

// Indirection so the compiler can't flag this as an obvious self-move
// at the call site; exercises the same code path a generic algorithm
// (e.g. std::swap-based reassignment) could trigger.
static void selfMoveAssign(Pool<>& pool) {
    pool = std::move(pool);
}

// Verifies self-move-assignment neither corrupts state nor crashes.
static void self_move_assignment_is_safe() {
    Pool<> pool(sizeof(void*), 3);
    void* p = pool.allocate();

    selfMoveAssign(pool);

    CHK(pool.totalBlocks() == 3);
    CHK(pool.usedBlocks() == 1);
    CHK(pool.owns(p));
}

// Executes all move-semantics test cases.
static void run_tests() {
    RUN(move_construct_transfers_state);
    RUN(move_construct_leaves_source_empty);
    RUN(move_assign_replaces_existing_allocation);
    RUN(self_move_assignment_is_safe);
}

REGISTER_TEST_SUITE();
