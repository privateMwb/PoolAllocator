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

#include <support/framework.h>

using namespace PoolPro;

namespace {
constexpr std::size_t kBlockSize = sizeof(void*);
constexpr std::size_t kBlockCount = 4096;
} // namespace

// Measures move-constructing a new pool from an existing one.
static void bench_move_construct() {
    Pool<false> src(kBlockSize, kBlockCount);
    (void)src.allocate();

    auto a = [&] {
        Pool<false> dst(std::move(src));
        doNotOptimize(&dst);
    };

    BENCH_SOLO("move-construct", a);
}

// Measures move-assignment, alternating direction each call so the
// same live pool state is ping-ponged between the two objects rather
// than draining into one side permanently.
static void bench_move_assign() {
    Pool<false> poolA(kBlockSize, kBlockCount);
    Pool<false> poolB(kBlockSize, kBlockCount);
    (void)poolA.allocate();
    bool flip = false;

    auto a = [&] {
        if (flip)
            poolA = std::move(poolB);
        else
            poolB = std::move(poolA);
        flip = !flip;
    };

    BENCH_SOLO("move-assign", a);
}

static void run_benchmarks() {
    bench_move_construct();
    std::cout << "\n";

    bench_move_assign();
}

REGISTER_BENCH_SUITE();
