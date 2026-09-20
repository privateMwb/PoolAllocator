# Benchmark Suite

This document describes the benchmark categories under `suite/` — what each
one measures, and the individual benchmarks it contains.

| Category | Focus |
|---|---|
| [Access](#access) | Read-only ownership checks and state queries on an already-populated pool |
| [Core](#core) | Allocating and releasing blocks individually or in batches, constructing, destroying, and resetting |
| [Lifecycle](#lifecycle) | Construction and moving |
| [Scaling](#scaling) | Cost vs. pool size, alignment, and exhaustion, independent of iteration count |
| [Utility](#utility) | Running allocation statistics |
| [Conventions](#conventions) | Registration, sizing, and elision conventions specific to the custom framework |

Every benchmark compares Pool against stdPool — a
`std::pmr::unsynchronized_pool_resource`, the standard library's own pool
allocator, and the conventional way this kind of allocation behavior is built
in C++. A category can support more than one standard for comparison, but for
now each category is benchmarked against a single standard.

Every `BENCH()` call, in every category below, is automatically repeated at
three iteration tiers — SMALL (10K), MEDIUM (100K), and LARGE (1M) — to
smooth out timing noise and show whether relative performance holds steady
as call volume increases. This applies uniformly across the whole suite; it
is not specific to any one category. The **Scaling** category below measures
something different: how per-operation cost changes as the pool's own
configuration changes, independent of iteration count.

Some benchmarks have no meaningful stdPool equivalent — a bare
`memory_resource` tracks no ownership, usage, or allocation statistics,
supports no batch operations, and isn't movable. Those run through
`BENCH_SOLO()` instead of `BENCH()`, timing Pool alone.

---

## Access

Benchmarks read-only operations against a pool that is already holding
allocations — ownership checks and querying current usage.

### Benchmarks

| File | What it covers |
|---|---|
| `ownership.cpp` | `owns()` hit and `owns()` miss (solo, no stdPool equivalent) |
| `state_query.cpp` | `usedBlocks()`, `freeBlocks()`, `totalBlocks()`, `capacity()`, and `blockStride()` (solo, no stdPool equivalent) |

---

## Core

Benchmarks the fundamental, most frequently exercised operations —
allocating and releasing blocks individually or in batches, constructing in
place, destroying, and reclaiming a pool via reset.

### Benchmarks

| File | What it covers |
|---|---|
| `allocate.cpp` | `allocate()` small, large, and over-aligned |
| `deallocate.cpp` | `deallocate()` returning a block to an otherwise full pool |
| `batch_allocate.cpp` | `allocateBatch()` filling a fixed-size batch (solo, no stdPool equivalent) |
| `batch_deallocate.cpp` | `deallocateBatch()` returning a fixed-size batch (solo, no stdPool equivalent) |
| `construct.cpp` | `create<T>()` with a trivial constructor, and with a non-trivial multi-argument constructor |
| `destroy.cpp` | `destroy<T>()` with a trivial destructor, and with a non-trivial destructor |
| `reset.cpp` | `reset()` alone, and `reset()` then refilling to a fixed block count, against stdPool's `release()` |

---

## Lifecycle

Benchmarks object lifetime operations — construction, destruction, and
moving. Pool has no copy constructor, so this category covers move only.

### Benchmarks

| File | What it covers |
|---|---|
| `construction.cpp` | Constructing an empty pool sized for N blocks — Pool allocates eagerly, stdPool defers to first use, so this compares two genuinely different construction strategies, not just two names for the same operation |
| `move.cpp` | Move construction, and move assignment ping-ponged between two populated pools (solo — `unsynchronized_pool_resource` is neither copyable nor movable) |

---

## Scaling

Benchmarks how per-operation cost changes as the pool's own configuration
changes — a separate axis from the SMALL/MEDIUM/LARGE iteration tiers
described above: those repeat the same fixed-size operation more times,
while Scaling grows the pool's block count or alignment requirement, or
removes its remaining headroom entirely, and observes the resulting cost.

### Benchmarks

| File | What it covers |
|---|---|
| `pool_size.cpp` | `allocate()` across increasing pool sizes: 1K, 100K, and 10M blocks |
| `alignment_scaling.cpp` | `allocate()` across increasing alignment requests: 4, 64, and 4096 bytes |
| `exhaustion.cpp` | `allocate()` with room to spare, and `allocate()` at capacity (failure path), against a bounded stdPool using a `std::pmr::monotonic_buffer_resource` over `std::pmr::null_memory_resource()` as its upstream |

---

## Utility

Benchmarks bookkeeping operations that don't belong to any of the categories
above — running allocation statistics.

### Benchmarks

| File | What it covers |
|---|---|
| `stats.cpp` | `getStats()` — total/current/peak usage and allocation count (solo, no stdPool equivalent) |

---

## Conventions

- **Registration** — every case is a `static void bench_<n>()` called from
  the file's `run_benchmarks()`, and the file ends with
  `REGISTER_BENCH_SUITE();`. `BENCH("label", c, s)` runs Pool (`c`) against
  stdPool (`s`); `BENCH_SOLO("label", a)` times Pool alone and is used only
  when stdPool has no equivalent.
- **Preventing elision** — every lambda passes its result to
  `doNotOptimize()` so the call can't be optimized away. A lambda with no
  result (`reset()`, `destroy()`, `deallocate()`) has nothing to pass.
- **Matching work** — the stdPool side must do the same work as the Pool
  side: `create<T>()` pairs with `allocate()` plus placement-new
  (`::new (raw) T(args...)`), `destroy<T>()` with a direct destructor call
  plus `deallocate()`, and `reset()` with `release()`.
- **Pool sizing** — every `BENCH()` repeats at the SMALL, MEDIUM, and LARGE
  tiers, so the pool is sized generously above the LARGE tier and never
  exhausts mid-run. Where that isn't practical (over-aligned blocks,
  sweeping pool size), Pool wraps around via `reset()` whenever it runs out,
  and stdPool calls `release()` every matching block count so its unbounded
  default upstream never accumulates more resident memory than Pool's own
  footprint.
- **Consumed inputs** — setup stays outside the lambda, except for operations
  that consume their input. `destroy.cpp`, `deallocate.cpp`, and
  `batch_deallocate.cpp` pre-build distinct objects or pointers and consume
  them one call at a time, since destroying an object or freeing a block
  twice is undefined behavior. `move.cpp` needs no rebuild: move cost is
  independent of how full the pool is, so moving an already-emptied pool
  exercises the same code path as moving a populated one.
