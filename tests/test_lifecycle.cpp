// Pool Object Lifecycle Test Suite
//
// Coverage:
// - Object construction
// - Object destruction
// - Constructor argument forwarding
// - Pool exhaustion during creation
// - Null pointer destruction
// - Multiple object lifecycle operations

#include "test_helper.h"

using namespace AllocatorPro;

// Verifies that create() constructs an object within the pool.
static void create_constructs_object() {
    Pool pool{sizeof(Tracker), 4};
    Tracker::reset();

    Tracker* t = pool.create<Tracker>(42);

    CHK(t != nullptr);
    CHK(t->value == 42);
    CHK(Tracker::constructions == 1);

    pool.destroy(t);
}

// Verifies that destroy() invokes the object's destructor and returns the block to the pool.
static void destroy_calls_destructor() {
    Pool pool{sizeof(Tracker), 4};
    Tracker::reset();

    Tracker* t = pool.create<Tracker>();
    pool.destroy(t);

    CHK(Tracker::destructions == 1);
    CHK(pool.freeBlocks() == 4);
}

// Verifies that create() perfectly forwards constructor arguments.
static void create_forwards_arguments() {
    struct Point {
        int x, y;
        Point(int x, int y) : x(x), y(y) {}
    };

    Pool pool{sizeof(Point), 4};
    Point* p = pool.create<Point>(3, 7);

    CHK(p != nullptr);
    CHK(p->x == 3);
    CHK(p->y == 7);

    pool.destroy(p);
}

// Verifies that create() returns nullptr when the pool has no free blocks remaining.
static void create_returns_nullptr_when_exhausted() {
    Pool pool{sizeof(Tracker), 2};
    Tracker::reset();

    Tracker* a = pool.create<Tracker>();
    Tracker* b = pool.create<Tracker>();
    Tracker* c = pool.create<Tracker>();

    CHK(a != nullptr);
    CHK(b != nullptr);
    CHK(c == nullptr);

    pool.destroy(a);
    pool.destroy(b);
}

// Verifies that destroying a nullptr is a safe no-op.
static void destroy_nullptr_is_safe() {
    Pool pool{sizeof(Tracker), 4};
    Tracker::reset();

    pool.destroy<Tracker>(nullptr);

    CHK(Tracker::destructions == 0);
}

// Verifies that multiple objects can be created and destroyed while correctly recycling pool blocks.
static void create_destroy_multiple() {
    Pool pool{sizeof(Tracker), 4};
    Tracker::reset();

    Tracker* a = pool.create<Tracker>(1);
    Tracker* b = pool.create<Tracker>(2);
    Tracker* c = pool.create<Tracker>(3);

    pool.destroy(a);
    pool.destroy(b);
    pool.destroy(c);

    CHK(Tracker::constructions == 3);
    CHK(Tracker::destructions == 3);
    CHK(pool.freeBlocks() == 4);
}

// Executes all object lifecycle test cases.
void run_lifecycle_tests() {
    setTitle("Object Lifecycle");

    RUN(create_constructs_object);
    RUN(destroy_calls_destructor);
    RUN(create_forwards_arguments);
    RUN(create_returns_nullptr_when_exhausted);
    RUN(destroy_nullptr_is_safe);
    RUN(create_destroy_multiple);

    std::cout << "\n";
}