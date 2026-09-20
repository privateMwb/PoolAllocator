// Pool deallocate() test suite.
//
// Coverage:
// - nullptr is silently ignored
// - A pointer not owned by the pool is silently ignored
// - An owned pointer is returned to the pool (usedBlocks()/freeBlocks() update)
// - A freed block is handed back out by the next allocate() (LIFO reuse)

#include <support/framework.h>

using namespace PoolPro;

// Verifies deallocate(nullptr) is a no-op.
static void ignores_null_pointer() {
    Pool<> pool(sizeof(void*), 4);
    pool.deallocate(nullptr);
    CHK(pool.usedBlocks() == 0);
}

// Verifies a foreign pointer is rejected without corrupting pool state.
static void ignores_foreign_pointer() {
    Pool<> pool(sizeof(void*), 4);
    int stackVar = 0;
    pool.deallocate(&stackVar);
    CHK(pool.freeBlocks() == 4);
}

// Verifies an owned pointer is released back to the pool.
static void returns_block_to_pool() {
    Pool<> pool(sizeof(void*), 4);
    void* p = pool.allocate();
    CHK(pool.usedBlocks() == 1);

    pool.deallocate(p);
    CHK(pool.usedBlocks() == 0);
    CHK(pool.freeBlocks() == 4);
}

// Verifies a freed block is the next one handed out.
static void freed_block_is_reusable() {
    Pool<> pool(sizeof(void*), 1);
    void* p = pool.allocate();
    pool.deallocate(p);

    void* reused = pool.allocate();
    CHK(reused == p);
}

// Executes all deallocate() test cases.
static void run_tests() {
    RUN(ignores_null_pointer);
    RUN(ignores_foreign_pointer);
    RUN(returns_block_to_pool);
    RUN(freed_block_is_reusable);
}

REGISTER_TEST_SUITE();
