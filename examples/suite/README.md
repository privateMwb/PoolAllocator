# Example Suite

This document describes the example categories under `suite/` — what each
one demonstrates, and the individual example files it contains.

| Category | Focus |
|---|---|
| [Advanced](#advanced) | Move semantics, exception safety, the fill/drain/reset cycle, and allocation-statistics tracking |
| [Integration](#integration) | Embedding the pool in a larger class, constructing non-trivial types, and backing smart pointers with a custom deleter |
| [Misuse](#misuse) | Common mistakes and the undefined behavior or contract violations they lead to, alongside the correct pattern |
| [Patterns](#patterns) | Bulk creation and batching, stats-driven sizing, one pool per thread, and steady-state alloc/free cycling |
| [Quickstart](#quickstart) | Construction, raw and typed allocation, object lifetime, and batch operations |
| [Conventions](#conventions) | Registration conventions specific to the custom framework |

Unlike the test suite, an example doesn't assert correctness — it
demonstrates real usage of the library, including deliberate misuse where
instructive (see [Misuse](#misuse)), so the reader sees both the correct
pattern and the mistake it guards against.

---

## Advanced

Demonstrates deeper mechanics of the library — move semantics, exception
safety, the fill/drain/reset cycle, and the optional allocation-statistics
tracking.

### Examples

| File | What it covers |
|---|---|
| `move_semantics.cpp` | Move construction and assignment, and what's actually safe to call on a moved-from pool |
| `exception_safety.cpp` | `create<T>()` when a constructor throws; the block is returned to the pool before the exception propagates |
| `reset_cycle.cpp` | Filling and draining the pool, and `reset()` as an O(1) return to the initial state without revisiting the watermark |
| `stats_tracking.cpp` | `Pool<true>`'s `totalAllocated_`, `allocations_`, `deallocations_`, and `peakUsed_`, and `reset()` clearing stats along with everything else |

---

## Integration

Demonstrates interoperability with the rest of a codebase — embedding the
pool inside a larger class, constructing non-trivial types, and backing
owning smart pointers with a custom deleter.

### Examples

| File | What it covers |
|---|---|
| `embedding_in_class.cpp` | Wrapping Pool as a private implementation detail behind a domain-specific API |
| `custom_types.cpp` | Forwarding constructor arguments through `create<T>()` for a multi-member type |
| `custom_deleter.cpp` | `std::unique_ptr` backed by Pool via a deleter that calls `destroy<T>()` instead of `delete` |

---

## Misuse

Demonstrates common mistakes and the undefined behavior or contract
violations they lead to, alongside the correct pattern — including examples
shown but not executed, so the reader can see what to avoid without the
program actually invoking undefined behavior.

### Examples

| File | What it covers |
|---|---|
| `out_of_space.cpp` | `allocate()` returning `nullptr` instead of throwing when capacity runs out |
| `dangling_after_reset.cpp` | A pointer left dangling by `reset()`, and the same block being handed to a different caller afterward |
| `destroy_unowned_ptr.cpp` | `destroy()`'s `owns(ptr)` precondition, and the no-op it produces if it's violated |
| `double_free.cpp` | The immediate double-free `AP_ASSERT(ptr != freeList_)` catches, and the double-free pattern it doesn't (shown, not executed) |

---

## Patterns

Demonstrates common usage idioms built on top of the core API — bulk object
creation and batching, stats-driven capacity sizing, one pool per thread,
and steady-state alloc/free cycling.

### Examples

| File | What it covers |
|---|---|
| `stats_driven_sizing.cpp` | Using `peakUsed_` from a representative workload to size a production pool |
| `bulk_struct_alloc.cpp` | `create<T>()` in a loop until the pool runs out, and `allocateBatch()` as the batch-oriented alternative |
| `pool_per_thread.cpp` | One pool per thread instead of synchronizing access to a shared one |
| `cycle_reuse_pattern.cpp` | The free-list idiom the pool is built around: allocate, use briefly, free, repeat |

---

## Quickstart

Demonstrates fundamental, everyday usage — construction, raw and typed
allocation, object lifetime, and batch operations.

### Examples

| File | What it covers |
|---|---|
| `basic_usage.cpp` | Construction, `allocate()`, `deallocate()`, `create<T>()`, `destroy()`, capacity/used/free/total, and `reset()` |
| `create_and_destroy.cpp` | Object lifetime across several `create<T>()` calls, and how `destroy()` returns the block immediately |
| `batch_allocation.cpp` | `allocateBatch()`/`deallocateBatch()`, and the partial count returned when a request can't be fully satisfied |

---

## Conventions

- **Registration** — every example file ends with
  `REGISTER_EXAMPLE_SUITE()`, which derives the suite's category from its
  containing directory and assigns it a sequential id within that category.
  This applies uniformly across every category above.
