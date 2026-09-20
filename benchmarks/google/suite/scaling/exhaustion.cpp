// PoolPro Scaling Benchmark Suite — Exhaustion
// Measures Pool allocate() performance against stdPool, with room to
// spare and at capacity.
//
// The "room to spare" case runs a fixed iteration count (kIterations)
// against a pool sized generously above it, so Pool never runs out
// mid-benchmark and only the success path is measured.
//
// unsynchronized_pool_resource has no fixed-buffer constructor the way
// monotonic_buffer_resource does, so stdPool can't be bounded to fail
// at an exact block count the way Pool can. It also isn't lazy about
// its own upstream the way one might expect: constructing it directly
// over std::pmr::null_memory_resource() throws immediately, since it
// pulls an initial chunk from upstream at construction, not on first
// allocate(). So the "at capacity" case instead gives it a small
// std::pmr::monotonic_buffer_resource (itself backed by
// null_memory_resource()) as a bounded upstream — enough room to
// construct and serve a handful of allocations, then reliably out of
// room for every call after, the same steady, always-fails shape as
// Pool returning nullptr on every call once its own fixed capacity is
// used up.
//
// Pool signals exhaustion by returning nullptr — no exception, no
// unwinding. stdPool's upstream throws std::bad_alloc instead, so its
// "at capacity" case must pay for a try/catch on every call just to
// have a comparable measurement at all. That cost difference is itself
// part of what this benchmark shows.
//
// Covers:
// - allocate() with room to spare
// - allocate() with no capacity left (failure path)

#include <benchmark/benchmark.h>
#include <support/framework.h>

#include <memory_resource>

using namespace PoolPro;

namespace {
constexpr std::size_t kBlockSize = sizeof(void*);
constexpr std::size_t kBlockCount = 2'000'000;
constexpr std::size_t kIterations = 1'000'000;
static_assert(kIterations <= kBlockCount, "pool must not be exhausted mid-benchmark");

constexpr std::size_t kUpstreamBufferSize = 4096;
} // namespace

// Measures allocate() with room to spare — the common case.
static void room_to_spare_pool(benchmark::State& state) {
    Pool<false> cSrc(kBlockSize, kBlockCount);

    for (auto _ : state) {
        void* p = cSrc.allocate();
        benchmark::DoNotOptimize(p);
    }
}
BENCHMARK(room_to_spare_pool)->Iterations(kIterations);

// stdPool counterpart of room_to_spare_pool.
static void room_to_spare_std(benchmark::State& state) {
    stdPool sSrc;

    for (auto _ : state) {
        void* p = sSrc.allocate(kBlockSize);
        benchmark::DoNotOptimize(p);
    }
}
BENCHMARK(room_to_spare_std)->Iterations(kIterations);

// Measures allocate() once there is no capacity left.
static void at_capacity_pool(benchmark::State& state) {
    Pool<false> cSrc(kBlockSize, kBlockCount);
    for (std::size_t i = 0; i < kBlockCount; ++i)
        (void)cSrc.allocate();

    for (auto _ : state) {
        void* p = cSrc.allocate(); // nullptr every call — no capacity left
        benchmark::DoNotOptimize(p);
    }
}
BENCHMARK(at_capacity_pool);

// stdPool counterpart of at_capacity_pool, paying for a try/catch on every call.
static void at_capacity_std(benchmark::State& state) {
    alignas(std::max_align_t) static std::byte upstreamBuffer[kUpstreamBufferSize];
    std::pmr::monotonic_buffer_resource boundedUpstream(upstreamBuffer, sizeof(upstreamBuffer),
                                                        std::pmr::null_memory_resource());
    stdPool sSrc(&boundedUpstream);

    // Drain the bounded upstream up front so every timed call below
    // hits the failure path — none of this setup cost is measured.
    try {
        for (;;)
            (void)sSrc.allocate(kBlockSize);
    } catch (const std::bad_alloc&) {
        // Expected once the bounded upstream runs out.
    }

    for (auto _ : state) {
        try {
            void* p = sSrc.allocate(kBlockSize);
            benchmark::DoNotOptimize(p);
        } catch (const std::bad_alloc&) {
            // Expected every call — the bounded upstream never has room.
        }
    }
}
BENCHMARK(at_capacity_std);
