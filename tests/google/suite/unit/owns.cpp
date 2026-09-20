// Pool owns() test suite.
//
// Coverage:
// - An allocated block is recognized as owned
// - A block that's been freed back to the free list is still owned
// - A pointer outside the pool's backing memory is rejected
// - A pointer inside the backing memory but not on a block boundary is rejected
// - nullptr is rejected

#include <gtest/gtest.h>

#include <PoolPro/Pool.h>

#include <cstddef>

using namespace PoolPro;

// Verifies a live allocation is recognized as owned.
TEST(Owns, OwnsAllocatedBlock) {
    Pool<> pool(sizeof(void*), 4);
    void* p = pool.allocate();
    EXPECT_TRUE(pool.owns(p));
}

// Verifies ownership is about backing memory, not allocation state:
// a freed block is still owned even though it's sitting on the free list.
TEST(Owns, OwnsFreeBlockInRange) {
    Pool<> pool(sizeof(void*), 4);
    void* p = pool.allocate();
    pool.deallocate(p);
    EXPECT_TRUE(pool.owns(p));
}

// Verifies a pointer entirely outside the pool's backing memory is rejected.
TEST(Owns, RejectsForeignPointer) {
    Pool<> pool(sizeof(void*), 4);
    int stackVar = 0;
    EXPECT_FALSE(pool.owns(&stackVar));
}

// Verifies a pointer inside the backing memory but not aligned to a
// block boundary is rejected.
TEST(Owns, RejectsMisalignedOffset) {
    Pool<> pool(sizeof(void*), 4);
    void* p = pool.allocate();
    auto* misaligned = static_cast<std::byte*>(p) + 1;
    EXPECT_FALSE(pool.owns(misaligned));
}

// Verifies nullptr is rejected.
TEST(Owns, RejectsNullptr) {
    Pool<> pool(sizeof(void*), 4);
    EXPECT_FALSE(pool.owns(nullptr));
}
