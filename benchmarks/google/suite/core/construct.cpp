// PoolPro Core Benchmark Suite — Construct
// Measures Pool create<T>() performance against stdPool,
// the standard library's own pool allocator, with placement new.
//
// Each pool owns a block count sized generously above the fixed
// iteration count (kIterations), so repeated calls never exhaust it
// mid-benchmark.
// std::pmr::unsynchronized_pool_resource has no typed construction
// helper, so its case allocates and placement-constructs by hand.
//
// Covers:
// - create<T>() with a trivial default constructor
// - create<T>() with a non-trivial, multi-argument constructor

#include <benchmark/benchmark.h>
#include <support/framework.h>

using namespace PoolPro;

namespace {
constexpr std::size_t kBlockCount = 2'000'000;
constexpr std::size_t kIterations = 1'000'000;
static_assert(kIterations <= kBlockCount, "pool must not be exhausted mid-benchmark");

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
static void construct_trivial_pool(benchmark::State& state) {
    Pool<false> cSrc(sizeof(Trivial), kBlockCount, alignof(Trivial));

    for (auto _ : state) {
        Trivial* p = cSrc.create<Trivial>();
        benchmark::DoNotOptimize(p);
    }
}
BENCHMARK(construct_trivial_pool)->Iterations(kIterations);

// Measures stdPool allocate + placement new with a trivial default constructor.
static void construct_trivial_std(benchmark::State& state) {
    stdPool sSrc;

    for (auto _ : state) {
        void* raw = sSrc.allocate(sizeof(Trivial), alignof(Trivial));
        Trivial* p = ::new (raw) Trivial();
        benchmark::DoNotOptimize(p);
    }
}
BENCHMARK(construct_trivial_std)->Iterations(kIterations);

// Measures create<T>() with a non-trivial, multi-argument constructor.
static void construct_nontrivial_pool(benchmark::State& state) {
    Pool<false> cSrc(sizeof(NonTrivial), kBlockCount, alignof(NonTrivial));

    for (auto _ : state) {
        NonTrivial* p = cSrc.create<NonTrivial>(7, 3.5);
        benchmark::DoNotOptimize(p);
    }
}
BENCHMARK(construct_nontrivial_pool)->Iterations(kIterations);

// Measures stdPool allocate + placement new with a non-trivial, multi-argument constructor.
static void construct_nontrivial_std(benchmark::State& state) {
    stdPool sSrc;

    for (auto _ : state) {
        void* raw = sSrc.allocate(sizeof(NonTrivial), alignof(NonTrivial));
        NonTrivial* p = ::new (raw) NonTrivial(7, 3.5);
        benchmark::DoNotOptimize(p);
    }
}
BENCHMARK(construct_nontrivial_std)->Iterations(kIterations);
