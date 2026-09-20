# Google Test Suite

This document describes the test categories under `suite/` — what each
one verifies, and the individual test files it contains.

| Category | Focus |
|---|---|
| [Concurrency](#concurrency) | Caller-supplied locking, lock-free concurrent observers, and one Pool per thread |
| [Integration](#integration) | Multiple operations working together end-to-end across realistic sequences |
| [Lifecycle](#lifecycle) | Construction, destruction, and moving |
| [Regression](#regression) | Previously fixed bugs and assert-only contracts pinned at their boundaries |
| [Unit](#unit) | Individual functions or methods in isolation |
| [Conventions](#conventions) | Registration and assertion conventions specific to Google Test |

Unlike the benchmark suite, tests validate the library's own correctness
directly — there is no reference implementation to compare against, so
results are simply pass or fail.

---

## Concurrency

Verifies thread-safety — concurrent reads and writes from multiple threads,
and correctness under simultaneous access. Pool has no internal
synchronization of its own, so these tests confirm the two patterns that
make that safe: a caller-supplied lock around shared access, and never
sharing a Pool across threads at all.

### Tests

| File | What it covers |
|---|---|
| `external_locking.cpp` | Concurrent `allocate()`/`deallocate()` calls and `allocateBatch()` calls, serialized by a caller-supplied mutex, never double-issue a block and total correctly |
| `concurrent_observers.cpp` | The const observers (`owns()`, `usedBlocks()`, `freeBlocks()`, `totalBlocks()`, `capacity()`, `getStats()`) stay consistent across concurrent, lock-free callers once state is settled |
| `pool_per_thread.cpp` | One Pool per thread, confirming independently-owned pools don't leak free-list or watermark state into one another |

---

## Integration

Verifies multiple operations working together end-to-end — for example,
`allocate()`, `deallocate()`, and `allocateBatch()` combined across a
realistic sequence — rather than a single function in isolation.

### Tests

| File | What it covers |
|---|---|
| `batch_freelist_drain.cpp` | `allocateBatch()` drains the free list first, then bump-allocates the remainder from virgin memory, across free-list-only, virgin-only, and mixed states |
| `alloc_reset_reuse.cpp` | Fill to capacity, then `reset()`, then reuse from block 0; `reset()` discards free-list state too, not just unused watermark capacity |
| `create_destroy_cycle.cpp` | `create()`/`destroy()` round trips free the slot for reuse; repeated churn across many rounds never leaks capacity |
| `stats_mixed_ops.cpp` | Stats stay correct across a mixed `allocate()`/`deallocate()`/`allocateBatch()`/`deallocateBatch()` sequence; peak usage never drops except via `reset()` |
| `watermark_freelist_order.cpp` | Freed blocks are always reused before the watermark advances into virgin memory, across a long interleaved sequence, and shared consistently between the single-call and batch APIs |

---

## Lifecycle

Verifies object lifetime operations — construction, destruction, and
moving.

### Tests

| File | What it covers |
|---|---|
| `construction.cpp` | A fresh pool starts fully available with every block free; `totalBlocks()` matches the request; every block is allocatable immediately, with no warm-up pass |
| `destruction.cpp` | Live, un-destroyed objects are not destructed when the pool itself is torn down; teardown is safe with a mix of allocated and free-listed blocks |
| `move_semantics.cpp` | Move construction and move assignment: full state transfer, moved-from pool left valid and empty, self-move-assignment safety |

---

## Regression

Verifies that a specific, previously fixed bug — or a deliberately
assert-only contract — stays exactly as intended. One test per resolved
issue or pinned contract, added at the time it's settled.

### Tests

| File | What it covers |
|---|---|
| `ctor_precondition_order.cpp` | Pins correct behavior at the exact boundary each constructor precondition allows (minimum blockCount, minimum blockSize), guarding against an off-by-one if a check is ever tightened |
| `alignment_contract.cpp` | The smallest (1) and a large power-of-two alignment are honored by every block in the pool, not just the first |
| `double_free_assert.cpp` | Legitimate reuse and consecutive frees of distinct blocks never false-positive against the immediate-double-free `AP_ASSERT` |

---

## Unit

Verifies individual functions or methods in isolation — the smallest
testable unit of behavior, independent of the categories above.

### Tests

| File | What it covers |
|---|---|
| `allocate.cpp` | Returns a pointer within capacity, `nullptr` when exhausted, `usedBlocks()`/`freeBlocks()` advance across calls, a freed block is reused (LIFO) before virgin memory is touched |
| `deallocate.cpp` | `nullptr` and foreign pointers are ignored; an owned pointer is returned to the pool and is the next one handed back out |
| `allocate_batch.cpp` | Fills the whole span when capacity allows, returns a partial count when exhausted, drains the free list before advancing the watermark |
| `deallocate_batch.cpp` | Owned pointers in the span are released in one call; `nullptr` and foreign pointers mixed in are skipped without corrupting the free list |
| `create.cpp` | Forwards constructor arguments, returns `nullptr` without constructing on a failed allocation, a throwing constructor still returns its block to the pool |
| `destroy.cpp` | Runs the object's destructor and reclaims its block via `deallocate()` |
| `reset.cpp` | Clears the free list and watermark, leaves `capacity()`/`totalBlocks()` unchanged, clears stats when enabled |
| `owns.cpp` | Live and free-listed blocks are owned; foreign pointers, misaligned offsets, and `nullptr` are not |
| `stats.cpp` | `totalAllocated_`, `allocations_`, `deallocations_`, and `peakUsed_` update correctly; a batch call updates them for every block it touches, in one shot |
| `block_introspection.cpp` | `capacity()`, `usedBlocks()`, `freeBlocks()`, `totalBlocks()`, and `blockStride()` stay mutually consistent through allocation churn |
| `alignment.cpp` | Default alignment matches `alignof(max_align_t)`; a custom alignment is honored; the stride rounds a non-aligned block size up correctly |

---

## Conventions

- **Registration** — every case is a `TEST(<Suite>, <Name>)`, which Google
  Test registers automatically at startup — there's no suite list to
  maintain by hand. Each file uses a single CamelCase suite name derived
  from its file name (e.g. `external_locking.cpp` → `ExternalLocking`,
  `allocate_batch.cpp` → `AllocateBatch`), and suite names are unique across
  files so every file links into one executable. There are no per-category
  sequential ids; run one file's tests with `--gtest_filter=<Suite>.*`. No
  file defines `main`; it is provided outside the test files. This applies
  uniformly across every category above.
- **Includes** — each file includes `<gtest/gtest.h>` and
  `<PoolPro/Pool.h>`.
- **Assertions** — checks use `EXPECT_*`, so a failure doesn't stop the rest
  of the test. `ASSERT_*` is used only where the test goes on to dereference
  or destroy a pointer that must be non-null.
- **Threads** — worker threads only record their results, in a
  `std::vector<char>` rather than a `std::vector<bool>` since each thread
  writes its own element. All assertions run on the main thread after the
  threads are joined.
