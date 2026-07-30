// Pool destruction test suite.
//
// Coverage:
// - A live object left in an allocated block is NOT destructed when the
//   pool itself is torn down (Pool frees raw memory only; it never tracks
//   or destructs objects on the caller's behalf)
// - This holds for multiple outstanding objects, not just one
// - Teardown is safe with a mix of still-allocated and free-listed blocks

#include <support/framework.h>

using namespace PoolPro;

namespace {

struct Tracked {
    bool* destroyedFlag;
    explicit Tracked(bool* flag) : destroyedFlag(flag) {
        *destroyedFlag = false;
    }
    ~Tracked() {
        *destroyedFlag = true;
    }
};

} // namespace

// Verifies the pool's own destructor does not run T's destructor for an
// object the caller never explicitly destroy()ed.
static void skips_object_destructor_on_pool_teardown() {
    bool destroyed = false;
    {
        Pool<> pool(sizeof(void*), 2);
        Tracked* t = pool.create<Tracked>(&destroyed);
        CHK(t != nullptr);
        // `pool` goes out of scope here without a matching destroy(t).
    }
    CHK(destroyed == false);
}

// Verifies the same holds across every outstanding object, not just one.
static void skips_destructors_for_all_outstanding_objects() {
    bool destroyedA = false;
    bool destroyedB = false;
    {
        Pool<> pool(sizeof(void*), 2);
        Tracked* a = pool.create<Tracked>(&destroyedA);
        Tracked* b = pool.create<Tracked>(&destroyedB);
        CHK(a != nullptr);
        CHK(b != nullptr);
    }
    CHK(destroyedA == false);
    CHK(destroyedB == false);
}

// Verifies teardown doesn't crash or assert when some blocks are still
// allocated and others are sitting on the free list at the same time.
static void teardown_with_mixed_free_and_used_blocks_is_safe() {
    Pool<> pool(sizeof(void*), 4);
    void* a = pool.allocate();
    void* b = pool.allocate();
    (void)pool.allocate();
    pool.deallocate(a);
    pool.deallocate(b);
    // `pool` is destroyed here with one block still allocated and two
    // blocks on the free list.
}

// Executes all destruction test cases.
static void run_tests() {
    RUN(skips_object_destructor_on_pool_teardown);
    RUN(skips_destructors_for_all_outstanding_objects);
    RUN(teardown_with_mixed_free_and_used_blocks_is_safe);
}

REGISTER_TEST_SUITE();
