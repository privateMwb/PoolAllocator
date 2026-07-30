# Example Suite

This document describes the example categories under `suite/` — what
each one demonstrates, and the individual example files it contains.

Unlike the test suite, an example doesn't assert correctness — it
demonstrates real usage of the library, including deliberate misuse
where instructive (see Misuse), so the reader sees both the correct
pattern and the mistake it guards against.

Every example file ends with `REGISTER_EXAMPLE_SUITE()`, which derives
the suite's category from its containing directory and assigns it a
sequential id within that category. This applies uniformly across
every category below.

---

## Advanced

Demonstrates deeper mechanics of the library — move semantics,
exception safety, the fill/drain/reset cycle, and the optional
allocation-statistics tracking.

### Examples

- `move_semantics.cpp` — move construction/assignment, and what's actually safe to call on a moved-from pool
- `exception_safety.cpp` — create<T>() when a constructor throws; the block is returned to the pool before the exception propagates
- `reset_cycle.cpp` — filling and draining the pool, and reset() as an O(1) return to the initial state without revisiting the watermark
- `stats_tracking.cpp` — Pool<true>'s totalAllocated_/allocations_/deallocations_/peakUsed_, and reset() clearing stats along with everything else

---

## Integration

Demonstrates interoperability with the rest of a codebase — embedding
the pool inside a larger class, constructing non-trivial types, and
backing owning smart pointers with a custom deleter.

### Examples

- `embedding_in_class.cpp` — wrapping Pool as a private implementation detail behind a domain-specific API
- `custom_types.cpp` — forwarding constructor arguments through create<T>() for a multi-member type
- `custom_deleter.cpp` — std::unique_ptr backed by Pool via a deleter that calls destroy<T>() instead of delete

---

## Misuse

Demonstrates common mistakes and the undefined behavior or contract
violations they lead to, alongside the correct pattern — including
examples shown but not executed, so the reader can see what to avoid
without the program actually invoking undefined behavior.

### Examples

- `out_of_space.cpp` — allocate() returning nullptr instead of throwing when capacity runs out
- `dangling_after_reset.cpp` — a pointer left dangling by reset(), and the same block being handed to a different caller afterward
- `destroy_unowned_ptr.cpp` — destroy()'s owns(ptr) precondition, and the no-op it produces if it's violated
- `double_free.cpp` — the immediate double-free AP_ASSERT(ptr != freeList_) catches, and the double-free pattern it doesn't (shown, not executed)

---

## Patterns

Demonstrates common usage idioms built on top of the core API — bulk
object creation and batching, stats-driven capacity sizing, one pool
per thread, and steady-state alloc/free cycling.

### Examples

- `stats_driven_sizing.cpp` — using peakUsed_ from a representative workload to size a production pool
- `bulk_struct_alloc.cpp` — create<T>() in a loop until the pool runs out, and allocateBatch() as the batch-oriented alternative
- `pool_per_thread.cpp` — one pool per thread instead of synchronizing access to a shared one
- `cycle_reuse_pattern.cpp` — the free-list idiom the pool is built around: allocate, use briefly, free, repeat

---

## Quickstart

Demonstrates fundamental, everyday usage — construction, raw and
typed allocation, object lifetime, and batch operations.

### Examples

- `basic_usage.cpp` — construction, allocate(), deallocate(), create<T>(), destroy(), capacity/used/free/total, reset()
- `create_and_destroy.cpp` — object lifetime across several create<T>() calls, and how destroy() returns the block immediately
- `batch_allocation.cpp` — allocateBatch()/deallocateBatch(), and the partial count returned when a request can't be fully satisfied
