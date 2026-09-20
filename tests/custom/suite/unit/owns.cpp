// Pool owns() test suite.
//
// Coverage:
// - An allocated block is recognized as owned
// - A block that's been freed back to the free list is still owned
// - A pointer outside the pool's backing memory is rejected
// - A pointer inside the backing memory but not on a block boundary is rejected
// - nullptr is rejected

#include <support/framework.h>

using namespace PoolPro;

// Verifies a live allocation is recognized as owned.
static void owns_allocated_block() {
    Pool<> pool(sizeof(void*), 4);
    void* p = pool.allocate();
    CHK(pool.owns(p));
}

// Verifies ownership is about backing memory, not allocation state:
// a freed block is still owned even though it's sitting on the free list.
static void owns_free_block_in_range() {
    Pool<> pool(sizeof(void*), 4);
    void* p = pool.allocate();
    pool.deallocate(p);
    CHK(pool.owns(p));
}

// Verifies a pointer entirely outside the pool's backing memory is rejected.
static void rejects_foreign_pointer() {
    Pool<> pool(sizeof(void*), 4);
    int stackVar = 0;
    CHK(!pool.owns(&stackVar));
}

// Verifies a pointer inside the backing memory but not aligned to a
// block boundary is rejected.
static void rejects_misaligned_offset() {
    Pool<> pool(sizeof(void*), 4);
    void* p = pool.allocate();
    auto* misaligned = static_cast<std::byte*>(p) + 1;
    CHK(!pool.owns(misaligned));
}

// Verifies nullptr is rejected.
static void rejects_nullptr() {
    Pool<> pool(sizeof(void*), 4);
    CHK(!pool.owns(nullptr));
}

// Executes all owns() test cases.
static void run_tests() {
    RUN(owns_allocated_block);
    RUN(owns_free_block_in_range);
    RUN(rejects_foreign_pointer);
    RUN(rejects_misaligned_offset);
    RUN(rejects_nullptr);
}

REGISTER_TEST_SUITE();
