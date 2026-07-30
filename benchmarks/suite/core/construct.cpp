// PoolPro Core Benchmark Suite — Construct
// Measures Pool create<T>() performance against stdPool,
// the standard library's own pool allocator, with placement new.
//
// Each pool owns a block count sized generously above the LARGE
// iteration tier, so repeated calls never exhaust it mid-benchmark.
// std::pmr::unsynchronized_pool_resource has no typed construction
// helper, so its case allocates and placement-constructs by hand.
//
// Covers:
// - create<T>() with a trivial default constructor
// - create<T>() with a non-trivial, multi-argument constructor

#include <support/framework.h>

using namespace PoolPro;

namespace {
constexpr std::size_t kBlockCount = 2'000'000;

struct Trivial {
    int a = 0;
    int b = 0;
};

struct NonTrivial {
    int a;
    double b;
    NonTrivial(int a_, double b_) : a(a_), b(b_) {}
};
} // namespace

// Measures create<T>() with a trivial default constructor.
static void bench_construct_trivial() {
    Pool<false> cSrc(sizeof(Trivial), kBlockCount, alignof(Trivial));
    stdPool sSrc;

    auto c = [&] {
        Trivial* p = cSrc.create<Trivial>();
        doNotOptimize(p);
    };

    auto s = [&] {
        void* raw = sSrc.allocate(sizeof(Trivial), alignof(Trivial));
        Trivial* p = ::new (raw) Trivial();
        doNotOptimize(p);
    };

    BENCH("create<T>() trivial", c, s);
}

// Measures create<T>() with a non-trivial, multi-argument constructor.
static void bench_construct_nontrivial() {
    Pool<false> cSrc(sizeof(NonTrivial), kBlockCount, alignof(NonTrivial));
    stdPool sSrc;

    auto c = [&] {
        NonTrivial* p = cSrc.create<NonTrivial>(7, 3.5);
        doNotOptimize(p);
    };

    auto s = [&] {
        void* raw = sSrc.allocate(sizeof(NonTrivial), alignof(NonTrivial));
        NonTrivial* p = ::new (raw) NonTrivial(7, 3.5);
        doNotOptimize(p);
    };

    BENCH("create<T>() non-trivial", c, s);
}

static void run_benchmarks() {
    bench_construct_trivial();
    std::cout << "\n";

    bench_construct_nontrivial();
}

REGISTER_BENCH_SUITE();
