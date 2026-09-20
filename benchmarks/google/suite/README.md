# Google Benchmark Suite

This document describes the benchmark categories under `suite/` — what each
one measures, and the individual benchmarks it contains.

| Category | Focus |
|---|---|
| [Access](#access) | Read-only ownership checks and state queries on an already-populated pool |
| [Core](#core) | Allocating and releasing blocks individually or in batches, constructing, destroying, and resetting |
| [Lifecycle](#lifecycle) | Construction and moving |
| [Scaling](#scaling) | Cost vs. pool size, alignment, and exhaustion, independent of iteration count |
| [Utility](#utility) | Running allocation statistics |
| [Conventions](#conventions) | Registration, sizing, and elision conventions specific to Google Benchmark |

Every benchmark compares Pool against stdPool — a
`std::pmr::unsynchronized_pool_resource`, the standard library's own pool
allocator, and the conventional way this kind of allocation behavior is built
in C++. A category can support more than one standard for comparison, but for
now each category is benchmarked against a single standard.

A paired comparison is registered as two Google Benchmark benchmarks that
share a base name: `<name>_pool` times Pool and `<name>_std` times stdPool.
Google Benchmark chooses how many iterations to run automatically, so there
are no fixed iteration tiers; benchmarks that consume blocks or pre-built
inputs pin an explicit iteration count instead (see **Conventions**). The
**Scaling** category below measures something different: how per-operation
cost changes as the pool's own configuration changes, independent of
iteration count.

Some benchmarks have no meaningful stdPool equivalent — a bare
`memory_resource` tracks no ownership, usage, or allocation statistics,
supports no batch operations, and isn't movable. Those are registered as a
single benchmark with no suffix, timing Pool alone.

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
changes — a separate axis from the iteration count: iterations repeat the
same fixed-size operation more times, while Scaling grows the pool's block
count or alignment requirement, or removes its remaining headroom entirely,
and observes the resulting cost.

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

- **Registration** — every case is a `static void <name>(benchmark::State& state)`
  followed by `BENCHMARK(<name>);` in the same file. Names carry no `BM_`
  prefix, and no file defines `BENCHMARK_MAIN()`; `main` is provided outside
  the benchmark files. Each file includes `<benchmark/benchmark.h>` and
  `support/framework.h`.
- **Pairing** — a paired comparison is two benchmarks, `<name>_pool` (Pool)
  and `<name>_std` (stdPool). A solo benchmark has a single function with no
  suffix and is used only when stdPool has no equivalent.
- **Iteration counts** — Google Benchmark picks iteration counts
  automatically. Benchmarks that consume blocks or pre-built inputs
  (`allocate.cpp`, `construct.cpp`, `destroy.cpp`, `deallocate.cpp`,
  `batch_allocate.cpp`, `batch_deallocate.cpp`, and the room-to-spare case in
  `exhaustion.cpp`) pin `->Iterations(kIterations)` (1M) instead, and a
  `static_assert` in each file checks that the pool or pre-built input is
  large enough to cover it. Everything else runs with automatic counts.
- **Preventing elision** — every loop body passes its result to
  `benchmark::DoNotOptimize()` so the call can't be optimized away. A loop
  with no result (`reset()`, `destroy()`, `deallocate()`) has nothing to pass.
- **Matching work** — the stdPool side must do the same work as the Pool
  side: `create<T>()` pairs with `allocate()` plus placement-new
  (`::new (raw) T(args...)`), `destroy<T>()` with a direct destructor call
  plus `deallocate()`, and `reset()` with `release()`.
- **Pool sizing** — pools that get consumed are sized generously above
  `kIterations`, so they never exhaust mid-run. Where that isn't practical
  (over-aligned blocks, sweeping pool size), Pool wraps around via `reset()`
  whenever it runs out, and stdPool calls `release()` every matching block
  count so its unbounded default upstream never accumulates more resident
  memory than Pool's own footprint. Those benchmarks keep automatic
  iteration counts.
- **Setup and consumed inputs** — setup stays outside the
  `for (auto _ : state)` loop and is not timed, except for operations that
  consume their input. `destroy.cpp`, `deallocate.cpp`, and
  `batch_deallocate.cpp` pre-build distinct objects or pointers and consume
  them one iteration at a time, since destroying an object or freeing a block
  twice is undefined behavior — which is why they pin their iteration count.
  `move.cpp` needs no rebuild: move cost is independent of how full the pool
  is, so moving an already-emptied pool exercises the same code path as
  moving a populated one.
