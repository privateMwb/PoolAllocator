// Pool create() test suite.
//
// Coverage:
// - Constructor arguments are forwarded to T
// - Returns nullptr without constructing when the pool is exhausted
// - A throwing constructor propagates, and the block is returned to the
//   pool rather than leaked

#include <support/framework.h>

#include <stdexcept>

using namespace PoolPro;

namespace {

struct Widget {
    int value;
    explicit Widget(int v) : value(v) {}
};

struct Thrower {
    explicit Thrower(bool doThrow) {
        if (doThrow)
            throw std::runtime_error("boom");
    }
};

} // namespace

// Verifies create() forwards its arguments into T's constructor.
static void forwards_constructor_arguments() {
    Pool<> pool(sizeof(void*), 4);
    Widget* w = pool.create<Widget>(42);
    CHK(w != nullptr);
    CHK(w->value == 42);
    pool.destroy(w);
}

// Verifies create() returns nullptr, without invoking T's constructor,
// once the pool has no blocks left.
static void returns_nullptr_when_exhausted() {
    Pool<> pool(sizeof(void*), 1);
    Widget* first = pool.create<Widget>(1);
    CHK(first != nullptr);

    Widget* second = pool.create<Widget>(2);
    CHK(second == nullptr);
}

// Verifies a throwing constructor propagates and the block is handed
// back to the pool instead of being lost.
static void throwing_constructor_returns_block() {
    Pool<> pool(sizeof(void*), 1);

    bool threw = false;
    try {
        (void)pool.create<Thrower>(true);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    CHK(threw);

    // If the block had been lost, this would fail with nullptr.
    Thrower* t = pool.create<Thrower>(false);
    CHK(t != nullptr);
    pool.destroy(t);
}

// Executes all create() test cases.
static void run_tests() {
    RUN(forwards_constructor_arguments);
    RUN(returns_nullptr_when_exhausted);
    RUN(throwing_constructor_returns_block);
}

REGISTER_TEST_SUITE();
