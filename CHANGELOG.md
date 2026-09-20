# Changelog

All notable changes to PoolAllocator are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Differential fuzz harness (`fuzz/fuzz_pool.cpp`) run under ASan and UBSan
  via ClusterFuzzLite on relevant pull requests and on a nightly schedule.
  See `FUZZING.md`.

## [1.0.0] - 2026-07-29

The first stable release of PoolAllocator, a fixed-capacity, free-list-based
memory pool for modern C++20.

### Added
- O(1) `allocate()` and `deallocate()` via an intrusive free list backed by a
  bump-pointer watermark: freed blocks are reused before untouched memory is
  ever handed out.
- `create<T>()` and `destroy<T>()` for in-place construction and destruction
  of arbitrary types. If a constructor throws, its block is returned to the
  pool rather than leaked.
- `allocateBatch()` and `deallocateBatch()` for handling many blocks in a
  single call, over `std::span<void*>`.
- `owns()` for O(1) pointer-ownership queries.
- `reset()` for O(1) bulk reclamation of the entire pool.
- Alignment-aware allocation, including alignments larger than a block's
  natural size.
- Optional allocation statistics (`Stats` / `getStats()`) via the compile-time
  `EnableStats` flag; when disabled, the statistics storage is zero-size and
  all bookkeeping is compiled out.
- Move construction and move assignment; a moved-from pool is left valid and
  empty (zero capacity).
- Introspection via `capacity()`, `usedBlocks()`, `freeBlocks()`,
  `totalBlocks()`, and `blockStride()`.
- Contract macros (`AP_PRE`, `AP_POST`, `AP_INVARIANT`, `AP_ASSERT`) for
  assert-driven precondition enforcement, compiled out when `NDEBUG` is
  defined.
- `rain::` namespace alias for `Pool`.

### Performance
- A single upfront buffer allocation eliminates per-block heap traffic for the
  pool's entire lifetime.
- The intrusive free list makes `deallocate()` and subsequent reuse O(1)
  pointer writes with no separate bookkeeping storage; virgin blocks are handed
  out by address arithmetic alone and are never written to or read from.
- Free-list pops prefetch the next node on GCC/Clang, so the following
  allocation is less likely to stall on a cache miss.
- `allocateBatch()` / `deallocateBatch()` amortize the cost of handling many
  blocks: the free list is written back once per batch, and virgin blocks are
  handed out in a branch-free loop with no dependent loads.
- `reset()` reclaims the whole pool in O(1), regardless of how many blocks were
  ever allocated.
- Exhaustion is a plain bounds check and a `nullptr` return — no exceptions,
  no reallocation.
- Alignment is resolved with bit arithmetic rather than a general
  modulo/division path.
- Benchmarked against a naive `new`/`delete`-per-block baseline at 10K / 100K /
  1M iterations; ahead of the baseline on every measured operation, with the
  largest wins on the exhaustion path (`allocate()` at capacity), bulk churn
  (`reset()` + refill), and large and custom-aligned allocations. Full
  results in `benchmarks/results/v1_0_0.md`.

### Testing
- Comprehensive test suite covering unit, integration, lifecycle, and
  regression tests; move semantics; exception safety; allocation failure
  handling; batch allocation and deallocation; free-list and watermark
  interaction; alignment behavior; statistics tracking; and external
  synchronization contracts.
- 99.4% line coverage and 100.0% function coverage, excluding test
  infrastructure and third-party dependencies.

### CI
- Automated builds and tests across GCC, Clang, MSVC, and AppleClang, each
  in Debug and Release configurations.

[Unreleased]: https://github.com/privateMwb/PoolAllocator/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/privateMwb/PoolAllocator/releases/tag/v1.0.0
