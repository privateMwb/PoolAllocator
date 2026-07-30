# Benchmark Suite

This document describes the benchmark categories under `suite/` — what
each one measures, and the individual benchmarks it contains.

Every benchmark compares Pool against stdPool — a
`std::pmr::unsynchronized_pool_resource`, the standard library's own
pool allocator, the conventional way this kind of allocation behavior
is built in C++. A category can support more than one standard for
comparison, but for now each category is benchmarked against a single
standard.

Every `BENCH()` call, in every category below, is automatically repeated
at three iteration tiers — SMALL (10K), MEDIUM (100K), and LARGE (1M) —
to smooth out timing noise and show whether relative performance holds
steady as call volume increases. This applies uniformly across the whole
suite; it is not specific to any one category. The **Scaling** category
below measures something different: how per-operation cost changes as
the pool's own configuration changes, independent of iteration count.

Some benchmarks have no meaningful stdPool equivalent — a bare
`memory_resource` tracks no ownership, usage, or allocation statistics,
supports no batch operations, and isn't movable. Those run through
`BENCH_SOLO()` instead of `BENCH()`, timing Pool alone.

---

## Access

Benchmarks read-only operations against a pool that is already holding
allocations — ownership checks and querying current usage.

### Benchmarks

- `ownership.cpp` — `owns()` hit, `owns()` miss (solo, no stdPool
  equivalent)
- `state_query.cpp` — `usedBlocks()`, `freeBlocks()`, `totalBlocks()`,
  `capacity()`, `blockStride()` (solo, no stdPool equivalent)

---

## Core

Benchmarks the fundamental, most frequently exercised operations —
allocating and releasing blocks individually or in batches, constructing
in place, destroying, and reclaiming a pool via reset.

### Benchmarks

- `allocate.cpp` — `allocate()` small, large, over-aligned (paired
  against stdPool)
- `deallocate.cpp` — `deallocate()` returning a block to an otherwise
  full pool (paired against stdPool)
- `batch_allocate.cpp` — `allocateBatch()` filling a fixed-size batch
  (solo, no stdPool equivalent)
- `batch_deallocate.cpp` — `deallocateBatch()` returning a fixed-size
  batch (solo, no stdPool equivalent)
- `construct.cpp` — `create<T>()` with a trivial constructor, a
  non-trivial multi-argument constructor (paired against stdPool)
- `destroy.cpp` — `destroy<T>()` with a trivial destructor, a
  non-trivial destructor (paired against stdPool)
- `reset.cpp` — `reset()` alone, `reset()` then refilling to a fixed
  block count (paired against stdPool's `release()`)

---

## Lifecycle

Benchmarks object lifetime operations — construction, destruction, and
moving. Pool has no copy constructor, so this category covers move
only.

### Benchmarks

- `construction.cpp` — constructing an empty pool sized for N blocks
  (paired against stdPool — Pool allocates eagerly, stdPool defers to
  first use, so this compares two genuinely different construction
  strategies, not just two names for the same operation)
- `move.cpp` — move-construct, move-assign (ping-ponged between two
  populated pools) (solo — `unsynchronized_pool_resource` is neither
  copyable nor movable)

---

## Scaling

Benchmarks how per-operation cost changes as the pool's own
configuration changes — a separate axis from the SMALL/MEDIUM/LARGE
iteration tiers described above: those repeat the same fixed-size
operation more times, while Scaling grows the pool's block count or
alignment requirement, or removes its remaining headroom entirely, and
observes the resulting cost.

### Benchmarks

- `pool_size.cpp` — `allocate()` across increasing pool sizes: 1K, 100K,
  10M blocks (paired against stdPool)
- `alignment_scaling.cpp` — `allocate()` across increasing alignment
  requests: 4, 64, 4096 bytes (paired against stdPool)
- `exhaustion.cpp` — `allocate()` with room to spare, `allocate()` at
  capacity (failure path) (paired against a bounded stdPool, using a
  `std::pmr::monotonic_buffer_resource` over
  `std::pmr::null_memory_resource()` as its upstream)

---

## Utility

Benchmarks bookkeeping operations that don't belong to any of the
categories above — running allocation statistics.

### Benchmarks

- `stats.cpp` — `getStats()` — total/current/peak usage, allocation
  count (solo, no stdPool equivalent)
