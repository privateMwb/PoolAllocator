# Pool Allocator

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue)](https://en.cppreference.com/w/cpp/23)
[![Status](https://img.shields.io/badge/status-learning%20project-green)](https://github.com/privateMwb/PoolAllocator)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

**PoolAllocator** is a from-scratch, fixed-size block allocator written in modern C++26. It was built as a deep dive into low-level memory management — embedded free list management, aligned slab allocation, object lifecycle control, and performance benchmarking against the heap.

---

## Table of Contents

- [Overview](#overview)
- [Motivation](#motivation)
- [Features](#features)
- [Quick Start](#quick-start)
- [Core API](#core-api)
- [Design Overview](#design-overview)
- [Complexity](#complexity)
- [Benchmarks](#benchmarks)
- [Project Structure](#project-structure)
- [Building from Source](#building-from-source)
- [Known Limitations](#known-limitations)
- [License](#license)

---

## Overview

`PoolAllocator::Pool` is a fixed-size block allocator built on a single contiguous slab with an embedded free list. It focuses on understanding how pool allocators work internally:

- Embedded free list management with no per-allocation metadata
- Aligned slab allocation via `::operator new`
- Object lifecycle control through typed `create<T>()` / `destroy<T>()`
- O(1) bulk reclamation via `reset()`

On top of this foundation, Pool adds batch allocation, ownership queries, optional zero-overhead debug statistics, and a benchmark suite comparing every operation against the heap (`new` / `delete`).

---

## Motivation

This project was built to understand:

- Fixed-size block allocation strategies
- Embedded free list management without external metadata
- Aligned slab allocation using `::operator new`
- Object lifecycle: construction and destruction within a pool
- Batch allocation patterns for throughput optimization
- Optional compile-time statistics with zero overhead when disabled
- Performance benchmarking vs. heap allocation

---

## Features

| Feature | Description |
|---|---|
| O(1) allocate/deallocate | Embedded free list pop/push — no heap traffic after construction |
| Aligned slab allocation | Configurable block size and alignment for the backing slab |
| Typed object creation | `create<T>()` forwards constructor arguments and placement-constructs into a block |
| Explicit destruction | `destroy<T>()` invokes the destructor and returns the block to the pool |
| Batch operations | `allocateBatch()` / `deallocateBatch()` over a `std::span<void*>` interface |
| O(1) bulk reset | `reset()` reclaims all blocks at once without calling destructors |
| Ownership queries | `owns()` checks whether a pointer belongs to the pool |
| Optional debug statistics | `Pool<true>` tracks allocation stats with zero overhead when disabled |
| Zero-cost disabled stats | `[[no_unique_address]]` on stats storage — no size penalty when stats are off |
| Move-only semantics | Move construction/assignment supported; copy is deleted |
| Constrained construction | `std::constructible_from` concept constraint on `create<T>()` |
| Contract annotations | Precondition contracts (`AP_PRE`) and purity annotations (`AP_PURE`) via `Contract.h` |

---

## Quick Start

### Basic allocation

```cpp
#include <PoolPro/Pool.h>

using namespace AllocatorPro;

int main() {
    Pool pool{64, 128};  // 128 blocks of 64 bytes

    void* p1 = pool.allocate();
    void* p2 = pool.allocate();

    pool.deallocate(p1);
    pool.deallocate(p2);
}
```

### Object lifecycle

```cpp
#include <PoolPro/Pool.h>

using namespace AllocatorPro;

struct Particle {
    float x, y, z;
    Particle(float x, float y, float z) : x(x), y(y), z(z) {}
    ~Particle() {}
};

int main() {
    Pool pool{sizeof(Particle), 64};

    Particle* p = pool.create<Particle>(1.0f, 2.0f, 3.0f);

    pool.destroy(p);   // destructor called, block returned to pool

    pool.reset();      // reclaim all blocks in O(1)
}
```

### Batch allocation

```cpp
#include <PoolPro/Pool.h>

using namespace AllocatorPro;

int main() {
    Pool pool{64, 32};
    void* blocks[32]{};

    std::size_t count = pool.allocateBatch(blocks);   // fill all 32 blocks
    pool.deallocateBatch(blocks);                     // return all at once
}
```

### Debug statistics

```cpp
#include <PoolPro/Pool.h>

using namespace AllocatorPro;

int main() {
    Pool<true> pool{64, 32};

    pool.allocate();
    pool.allocate();

    const auto& s = pool.getStats();
    // s.allocations_, s.totalAllocated_, s.peakUsed_, s.deallocations_
}
```

---

## Core API

### Constructors

```cpp
Pool pool{blockSize, blockCount};                    // default alignment
Pool pool{blockSize, blockCount, alignment};         // custom alignment
Pool b{std::move(a)};                                // move construction
b = std::move(a);                                    // move assignment
```

### Core allocation

```cpp
[[nodiscard]] void* allocate() noexcept;
void deallocate(void* ptr) noexcept;
```

### Batch allocation

```cpp
[[nodiscard]] std::size_t allocateBatch(std::span<void*> out) noexcept;
void deallocateBatch(std::span<void*> ptrs) noexcept;
```

### Object lifecycle

```cpp
template<typename T, typename... Args>
requires std::constructible_from<T, Args...>
[[nodiscard]] T* create(Args&&... args);

template<typename T>
void destroy(T* ptr) noexcept;
```

### Pool management

```cpp
void reset() noexcept;
```

### Introspection

```cpp
[[nodiscard]] bool owns(const void* ptr) const noexcept;

[[nodiscard]] const Stats& getStats() const noexcept requires EnableStats;

[[nodiscard]] std::size_t capacity()    const noexcept;
[[nodiscard]] std::size_t usedBlocks()  const noexcept;
[[nodiscard]] std::size_t freeBlocks()  const noexcept;
[[nodiscard]] std::size_t totalBlocks() const noexcept;
[[nodiscard]] std::size_t blockStride() const noexcept;
```

---

## Design Overview

Pool uses a single contiguous heap-allocated slab with an embedded singly-linked free list.

### Internal layout

```
memory_ (pointer)
  ↓
[block 0][block 1][block 2][block 3][...]
  ↓         ↓
FreeNode  FreeNode
  next →    next → nullptr
```

- **`memory_`** — pointer to raw allocated slab
- **`blockSize_`** — size of each block in bytes
- **`stride_`** — aligned size between block starts
- **`blockCount_`** — total number of blocks
- **`freeList_`** — head of the embedded free list
- **`freeBlockCount_`** — current number of free blocks
- **`stats_`** — optional debug statistics (zero-size when disabled)

### Allocation strategy

Allocation pops the head of the free list:

```cpp
FreeNode* block = freeList_;
freeList_       = freeList_->next;
--freeBlockCount_;
return block;
```

No heap traffic after construction. No per-allocation metadata.

### Deallocation strategy

Deallocation pushes the block back onto the free list:

```cpp
auto* node = static_cast<FreeNode*>(ptr);
node->next  = freeList_;
freeList_   = node;
++freeBlockCount_;
```

### Free list initialization

On construction and reset, every block is linked into the free list:

```
block 0 → block 1 → block 2 → ... → block N-1 → nullptr
```

### Stride computation

Stride is computed as the alignment-padded block size:

```cpp
stride_ = alignForward(blockSize, alignment);
```

This ensures every block starts at a correctly aligned address.

### Object lifecycle

`create<T>()` allocates a block and placement-constructs the object:

```cpp
T* obj = pool.create<T>(args...);
```

`destroy<T>()` invokes the destructor and returns the block to the pool:

```cpp
pool.destroy(obj);
```

### Optional statistics

Statistics are controlled at compile time via the `EnableStats` template parameter:

```cpp
Pool<false> pool{64, 128};        // no stats — zero overhead
Pool<true>  debug{64, 128};       // stats enabled
```

`[[no_unique_address]]` ensures the stats struct occupies zero bytes when disabled.

### Exception safety model

- `allocate()` returns `nullptr` on exhaustion — no exceptions
- `create<T>()` returns `nullptr` if allocation fails
- Move operations are `noexcept`
- `reset()`, `deallocate()`, `destroy<T>()` are `noexcept`
- Double-free is undefined behaviour — `owns()` guards against foreign pointers

---

## Complexity

### Time complexity

| Operation | Complexity | Notes |
|---|---|---|
| `allocate` | O(1) | Free list pop |
| `deallocate` | O(1) | Free list push |
| `allocateBatch` | O(n) | n = batch size |
| `deallocateBatch` | O(n) | n = batch size |
| `create<T>` | O(1) | Allocation + placement construction |
| `destroy<T>` | O(1) | Destructor invocation + free list push |
| `reset` | O(n) | Relinks all blocks into free list |
| `owns` | O(1) | Bounds check + stride alignment check |
| `getStats` | O(1) | Reference return |

### Space complexity

- O(n) for the backing slab (`stride * blockCount` bytes)
- O(1) for all metadata
- O(0) for stats when `EnableStats = false`

### Notes

- No per-allocation overhead — `FreeNode` is embedded directly in free blocks
- Blocks must be at least `sizeof(void*)` bytes to hold a `FreeNode`
- `reset()` does not call destructors — caller is responsible for object cleanup
- Double-free is undefined behaviour; `deallocate` rejects foreign pointers via `owns()`

---

## Benchmarks

Benchmarks compare `Pool` against heap (`new` / `delete`) across all operations. All times are total elapsed time for the listed iteration count.

> Compiled with `-std=c++26`. Results may vary depending on hardware and compiler optimizations.

<details>
<summary>Show benchmark results</summary>

#### Constructor

```
----------------------------------------------------------------------
Constructor Benchmarks                  Time           Iteration
----------------------------------------------------------------------
Pool Construct                          265.95 ms       1000000
Heap Construct                          142.49 ms       1000000

Pool Move Construct                     172.37 ms       1000000
Heap Move Construct                     140.73 ms       1000000

Pool Move Assign                        330.23 ms       1000000
Heap Move Assign                        301.36 ms       1000000
----------------------------------------------------------------------
```

#### Allocation

```
----------------------------------------------------------------------
Allocation Benchmarks                   Time           Iteration
----------------------------------------------------------------------
Pool Allocate                           3.18 ms         1000000
Heap Allocate                           160.17 ms       1000000

Pool Alloc Dealloc Cycle                8.99 ms         1000000
Heap Alloc Dealloc Cycle                280.40 ms       1000000
----------------------------------------------------------------------
```

#### Bulk Allocation

```
----------------------------------------------------------------------
Bulk Allocation Benchmarks              Time           Iteration
----------------------------------------------------------------------
Pool Batch Allocate                     8.45 ms         1000000
Heap Batch Allocate                     3.92 s          1000000

Pool Batch Deallocate                   60.65 ms        1000000
Heap Batch Deallocate                   3.55 s          1000000
----------------------------------------------------------------------
```

#### Object Lifecycle

```
----------------------------------------------------------------------
Object Lifecycle Benchmarks             Time           Iteration
----------------------------------------------------------------------
Pool Create                             4.16 ms         1000000
Heap Create                             526.46 us       1000000

Pool Destroy                            4.23 ms         1000000
Heap Destroy                            526.46 us       1000000

Pool Create Destroy Cycle               8.45 ms         1000000
Heap Create Destroy Cycle               526.46 us       1000000
----------------------------------------------------------------------
```

#### Reset

```
----------------------------------------------------------------------
Reset Benchmarks                        Time           Iteration
----------------------------------------------------------------------
Pool Reset                              553.53 ms       1000000
Pool Manual Deallocate                  1.05 s          1000000
Heap Reset                              38.23 s         1000000
----------------------------------------------------------------------
```

#### Throughput

```
----------------------------------------------------------------------
Throughput Benchmarks                   Time           Iteration
----------------------------------------------------------------------
Pool Fill Drain                         1.05 s          1000000
Heap Fill Drain                         37.43 s         1000000

Pool Interleaved                        3.18 ms         1000000
Heap Interleaved                        124.32 ms       1000000
----------------------------------------------------------------------
```

#### Summary

Pool dominates wherever bulk patterns or high-frequency fixed-size allocation is involved. Single allocation (`Pool Allocate`: 3.18 ms vs `Heap Allocate`: 160.17 ms) shows a 50x advantage — free list pop vs heap search with no fragmentation overhead.

Bulk allocation tells the same story. Batch allocate (`Pool Batch Allocate`: 8.45 ms vs `Heap Batch Allocate`: 3.92 s) and batch deallocate (`Pool Batch Deallocate`: 60.65 ms vs `Heap Batch Deallocate`: 3.55 s) show 40–58x advantages because the pool operates entirely within a single contiguous slab with no system calls.

Throughput benchmarks reinforce this — fill/drain cycles (`Pool Fill Drain`: 1.05 s vs `Heap Fill Drain`: 37.43 s) show a 35x advantage, and interleaved alloc/dealloc (`Pool Interleaved`: 3.18 ms vs `Heap Interleaved`: 124.32 ms) show a 39x advantage.

Reset is O(n) for the pool (relinking all blocks) vs O(n) individual heap deletes. Pool Reset (553 ms) beats Heap Reset (38.23 s) by 69x — the same asymptotic complexity but with dramatically lower constant factors from cache locality and no system calls.

Heap wins on object lifecycle with trivially constructible types. `Pool Create` (4.16 ms) vs `Heap Create` (526 us) favors heap because `Particle` has no constructor overhead, so the pool gains nothing from avoiding heap search while still paying the `create` wrapper cost.

| Category | Winner | Notes |
|---|---|---|
| Single allocate | Pool | 50x faster — free list pop vs heap search |
| Alloc/dealloc cycle | Pool | 31x faster — no fragmentation overhead |
| Batch allocate | Pool | 464x faster — contiguous slab vs N heap calls |
| Batch deallocate | Pool | 58x faster — free list push vs N heap frees |
| Fill/drain throughput | Pool | 35x faster — slab locality vs heap fragmentation |
| Interleaved | Pool | 39x faster — O(1) push/pop vs heap overhead |
| Reset | Pool | 69x faster — slab relink vs N individual deletes |
| Object lifecycle | Heap | Trivial types gain nothing from pool create/destroy |
| Construction | Heap | Slab allocation cost exceeds heap for small pools |

**Use Pool when:** objects are fixed-size, allocation frequency is high, or bulk patterns are needed.
**Use heap when:** object sizes vary, lifetimes are independent, or object count is very small.

</details>

---

## Project Structure

```
PoolAllocator/
├── include/
│   └── PoolPro/
│       ├── Contract.h
│       ├── Pool.h
│       └── Pool.tpp
│
├── tests/
├── benchmarks/
├── examples/
│
├── cmake/
│   └── PoolProConfig.cmake.in
│
├── .gitignore
├── CMakeLists.txt
├── README.md
└── LICENSE
```

---

## Building from Source

### Requirements

- GCC 16+ or Clang with C++26 support
- CMake 3.20+

### Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### Run tests

```bash
./tests                 # run all test suites
./tests list            # list available suites
./tests 1               # run by number
./tests name            # run by name
```

### Run benchmarks

```bash
./benchmarks            # run all benchmark suites
./benchmarks list       # list available suites
./benchmarks 1          # run by number
./benchmarks name       # run by name
```

### Run examples

```bash
./examples              # run all examples
./examples list         # list available examples
./examples 1            # run by number
./examples name         # run by name
```

---

## Known Limitations

- **Pool loses to raw heap allocation for construction and object lifecycle with trivially constructible types** (e.g. `Pool Create`: `4.16 ms` vs `Heap Create`: `526.46 us` at 1,000,000 iterations) — the `create()` wrapper cost isn't offset when the type has no meaningful constructor overhead for the pool to save on. See [Benchmarks](#benchmarks).
- **`reset()` does not call destructors.** The caller is responsible for destroying live objects before calling `reset()`; skipping this leaks any resources those objects own, without ever running their destructors.
- **Double-free of a valid block is undefined behaviour.** `deallocate()` rejects pointers that don't belong to the pool via `owns()`, but it does not detect a double-free of a pointer that was already returned to the free list.
- **Blocks must be at least `sizeof(void*)` bytes**, enforced via an `AP_PRE` contract — the embedded free list needs room for a `FreeNode` pointer, so smaller block sizes are rejected.
- **`getStats()` is only callable on `Pool<true>`.** Calling it on a `Pool<false>` instance is a compile error rather than a runtime one, since statistics are compiled out entirely when disabled.

---

## License

Licensed under the [MIT License](LICENSE) — free to use, modify, and distribute for educational and personal purposes.
