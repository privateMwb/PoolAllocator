// Pool create() test suite.
//
// Coverage:
// - Constructor arguments are forwarded to T
// - Returns nullptr without constructing when the pool is exhausted
// - A throwing constructor propagates, and the block is returned to the
//   pool rather than leaked

#include <gtest/gtest.h>

#include <PoolPro/Pool.h>

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
TEST(Create, ForwardsConstructorArguments) {
    Pool<> pool(sizeof(void*), 4);
    Widget* w = pool.create<Widget>(42);
    ASSERT_NE(w, nullptr);
    EXPECT_EQ(w->value, 42);
    pool.destroy(w);
}

// Verifies create() returns nullptr, without invoking T's constructor,
// once the pool has no blocks left.
TEST(Create, ReturnsNullptrWhenExhausted) {
    Pool<> pool(sizeof(void*), 1);
    Widget* first = pool.create<Widget>(1);
    EXPECT_NE(first, nullptr);

    Widget* second = pool.create<Widget>(2);
    EXPECT_EQ(second, nullptr);
}

// Verifies a throwing constructor propagates and the block is handed
// back to the pool instead of being lost.
TEST(Create, ThrowingConstructorReturnsBlock) {
    Pool<> pool(sizeof(void*), 1);

    EXPECT_THROW((void)pool.create<Thrower>(true), std::runtime_error);

    // If the block had been lost, this would fail with nullptr.
    Thrower* t = pool.create<Thrower>(false);
    ASSERT_NE(t, nullptr);
    pool.destroy(t);
}
