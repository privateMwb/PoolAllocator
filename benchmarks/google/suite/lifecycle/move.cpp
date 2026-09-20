// PoolPro Lifecycle Benchmark Suite — Move
// Measures Pool move-construct and move-assign performance.
//
// std::pmr::unsynchronized_pool_resource is neither copyable nor
// movable, so both cases run solo. Move cost is independent of how
// full the pool is — it exchanges a fixed set of scalars and pointers,
// never touches a block — so moving an already-emptied (moved-from)
// pool exercises the identical code path as moving a populated one.
//
// Covers:
// - move-construct from a populated pool
// - move-assign, ping-ponged between two populated pools

#include <benchmark/benchmark.h>
#include <support/framework.h>

using namespace PoolPro;

namespace {
constexpr std::size_t kBlockSize = sizeof(void*);
constexpr std::size_t kBlockCount = 4096;
} // namespace

// Measures move-constructing a new pool from an existing one.
static void move_construct(benchmark::State& state) {
    Pool<false> src(kBlockSize, kBlockCount);
    (void)src.allocate();

    for (auto _ : state) {
        Pool<false> dst(std::move(src));
        benchmark::DoNotOptimize(&dst);
    }
}
BENCHMARK(move_construct);

// Measures move-assignment, alternating direction each call so the
// same live pool state is ping-ponged between the two objects rather
// than draining into one side permanently.
static void move_assign(benchmark::State& state) {
    Pool<false> poolA(kBlockSize, kBlockCount);
    Pool<false> poolB(kBlockSize, kBlockCount);
    (void)poolA.allocate();
    bool flip = false;

    for (auto _ : state) {
        if (flip)
            poolA = std::move(poolB);
        else
            poolB = std::move(poolA);
        flip = !flip;
    }
}
BENCHMARK(move_assign);
