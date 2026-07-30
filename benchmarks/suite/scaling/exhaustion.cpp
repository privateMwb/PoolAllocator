// PoolPro Scaling Benchmark Suite — Exhaustion
// Measures Pool allocate() performance against stdPool, with room to
// spare and at capacity.
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

#include <support/framework.h>

#include <memory_resource>

using namespace PoolPro;

namespace {
constexpr std::size_t kBlockSize = sizeof(void*);
constexpr std::size_t kBlockCount = 4096;
constexpr std::size_t kUpstreamBufferSize = 4096;
} // namespace

// Measures allocate() with room to spare — the common case.
static void bench_room_to_spare() {
    Pool<false> cSrc(kBlockSize, kBlockCount);
    stdPool sSrc;

    auto c = [&] {
        void* p = cSrc.allocate();
        doNotOptimize(p);
    };

    auto s = [&] {
        void* p = sSrc.allocate(kBlockSize);
        doNotOptimize(p);
    };

    BENCH("allocate() room to spare", c, s);
}

// Measures allocate() once there is no capacity left.
static void bench_at_capacity() {
    Pool<false> cSrc(kBlockSize, kBlockCount);
    for (std::size_t i = 0; i < kBlockCount; ++i)
        (void)cSrc.allocate();

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

    auto c = [&] {
        void* p = cSrc.allocate(); // nullptr every call — no capacity left
        doNotOptimize(p);
    };

    auto s = [&] {
        try {
            void* p = sSrc.allocate(kBlockSize);
            doNotOptimize(p);
        } catch (const std::bad_alloc&) {
            // Expected every call — the bounded upstream never has room.
        }
    };

    BENCH("allocate() at capacity", c, s);
}

static void run_benchmarks() {
    bench_room_to_spare();
    std::cout << "\n";

    bench_at_capacity();
}

REGISTER_BENCH_SUITE();
