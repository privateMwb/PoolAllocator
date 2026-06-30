# PoolAllocator

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue)](https://en.cppreference.com/w/cpp/23)
[![Status](https://img.shields.io/badge/status-learning%20project-green)](https://github.com/privateMwb/PoolAllocator)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

A custom C++ pool allocator implementation built for learning low-level memory management, fixed-size block allocation, free list management, and performance benchmarking.

---

## Table of Contents

- [Overview](#overview)
- [Motivation / Goals](#motivation--goals)
- [Features](#features)
- [Design Overview](#design-overview)
- [Complexity](#complexity)
- [Quick Example](#quick-example)
- [Core API](#core-api)
- [Benchmark Results](#benchmark-results)
- [Project Structure](#project-structure)
- [Build Instructions](#build-instructions)
- [Notes](#notes)
- [License](#license)

---

## Overview

PoolAllocator (`Pool`) is a fixed-size block allocator implemented from scratch in modern C++ (C++26).
It focuses on understanding how pool allocators work internally, including embedded free list management, aligned slab allocation, and object lifecycle control.

It also includes:

- Typed object construction and destruction via `create<T>()` / `destroy<T>()`
- Batch allocation and deallocation via `allocateBatch()` / `deallocateBatch()`
- O(1) bulk reset via `reset()`
- Ownership queries via `owns()`
- Optional debug statistics via `Pool<true>`
- Benchmark suite comparing against heap (`new` / `delete`)
- Unit tests for correctness validation

---

## Motivation / Goals

This project was built to understand:

- Fixed-size block allocation strategies
- Embedded free list management without external metadata
- Aligned slab allocation using `::operator new`
- Object lifecycle: construction and destruction within a pool
- Batch allocation patterns for throughput optimization
- Optional compile-time statistics with zero overhead when disabled
- Performance benchmarking vs heap allocation

---

## Features

- O(1) allocation and deallocation via embedded free list
- Aligned slab allocation with configurable block size and alignment
- Typed object creation with forwarded constructor arguments via `create<T>()`
- Explicit destructor invocation via `destroy<T>()`
- Batch allocation and deallocation via `std::span<void*>` interface
- Full reset to reclaim all blocks in O(1) without calling destructors
- Ownership query via `owns()`
- Optional debug statistics via `Pool<true>` with zero overhead when disabled
- `[[no_unique_address]]` on stats storage — no size penalty when stats are off
- Move semantics with deleted copy
- `std::constructible_from` concept constraint on `create<T>()`
- Precondition contracts (`AP_PRE`) and purity annotations (`AP_PURE`) via `Contract.h`

---

## Design Overview

Pool uses a single contiguous heap-allocated slab with an embedded singly-linked free list.

### Internal Structure

```
memory_ (pointer)
  ↓
[block 0][block 1][block 2][block 3][...]
  ↓         ↓
FreeNode  FreeNode
  next →    next → nullptr
```

- `memory_` → pointer to raw allocated slab
- `blockSize_` → size of each block in bytes
- `stride_` → aligned size between block starts
- `blockCount_` → total number of blocks
- `freeList_` → head of the embedded free list
- `freeBlockCount_` → current number of free blocks
- `stats_` → optional debug statistics (zero-size when disabled)

### Allocation Strategy

Allocation pops the head of the free list:

```cpp
FreeNode* block = freeList_;
freeList_       = freeList_->next;
--freeBlockCount_;
return block;
```

No heap traffic after construction. No per-allocation metadata.

### Deallocation Strategy

Deallocation pushes the block back onto the free list:

```cpp
auto* node = static_cast<FreeNode*>(ptr);
node->next  = freeList_;
freeList_   = node;
++freeBlockCount_;
```

### Free List Initialization

On construction and reset, every block is linked into the free list:

```
block 0 → block 1 → block 2 → ... → block N-1 → nullptr
```

### Stride Computation

Stride is computed as the alignment-padded block size:

```cpp
stride_ = alignForward(blockSize, alignment);
```

This ensures every block starts at a correctly aligned address.

### Object Lifecycle

`create<T>()` allocates a block and placement-constructs the object:

```cpp
T* obj = pool.create<T>(args...);
```

`destroy<T>()` invokes the destructor and returns the block to the pool:

```cpp
pool.destroy(obj);
```

### Optional Statistics

Statistics are controlled at compile time via the `EnableStats` template parameter:

```cpp
Pool<false> pool{64, 128};        // no stats — zero overhead
Pool<true>  debug{64, 128};       // stats enabled
```

`[[no_unique_address]]` ensures the stats struct occupies zero bytes when disabled.

### Exception Safety Model

- `allocate()` returns `nullptr` on exhaustion — no exceptions
- `create<T>()` returns `nullptr` if allocation fails
- Move operations are `noexcept`
- `reset()`, `deallocate()`, `destroy<T>()` are `noexcept`
- Double-free is undefined behaviour — `owns()` guards against foreign pointers

---

## Complexity

### Time Complexity

| Operation         | Complexity | Notes                                        |
| ----------------- | ---------- | -------------------------------------------- |
| `allocate`        | O(1)       | Free list pop                                |
| `deallocate`      | O(1)       | Free list push                               |
| `allocateBatch`   | O(n)       | n = batch size                               |
| `deallocateBatch` | O(n)       | n = batch size                               |
| `create<T>`       | O(1)       | Allocation + placement construction          |
| `destroy<T>`      | O(1)       | Destructor invocation + free list push       |
| `reset`           | O(n)       | Relinks all blocks into free list            |
| `owns`            | O(1)       | Bounds check + stride alignment check        |
| `getStats`        | O(1)       | Reference return                             |

### Space Complexity

- O(n) for the backing slab (`stride * blockCount` bytes)
- O(1) for all metadata
- O(0) for stats when `EnableStats = false`

### Notes

- No per-allocation overhead — `FreeNode` is embedded directly in free blocks
- Blocks must be at least `sizeof(void*)` bytes to hold a `FreeNode`
- `reset()` does not call destructors — caller is responsible for object cleanup
- Double-free is undefined behaviour; `deallocate` rejects foreign pointers via `owns()`

---

## Quick Example

### Basic Allocation

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

### Object Lifecycle

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

### Batch Allocation

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

### Debug Statistics

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

### Core Allocation

```cpp
[[nodiscard]] void* allocate() noexcept;
void deallocate(void* ptr) noexcept;
```

### Batch Allocation

```cpp
[[nodiscard]] std::size_t allocateBatch(std::span<void*> out) noexcept;
void deallocateBatch(std::span<void*> ptrs) noexcept;
```

### Object Lifecycle

```cpp
template<typename T, typename... Args>
requires std::constructible_from<T, Args...>
[[nodiscard]] T* create(Args&&... args);

template<typename T>
void destroy(T* ptr) noexcept;
```

### Pool Management

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

## Benchmark Results

Benchmarks compare `Pool` against heap (`new` / `delete`) across all operations.
All times are total elapsed time for the listed iteration count.

> Compiled with `-std=c++26`. Results may vary depending on hardware and compiler optimizations.

### Constructor

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

### Allocation

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

### Bulk Allocation

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

### Object Lifecycle

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

### Reset

```
----------------------------------------------------------------------
Reset Benchmarks                        Time           Iteration
----------------------------------------------------------------------
Pool Reset                              553.53 ms       1000000
Pool Manual Deallocate                  1.05 s          1000000
Heap Reset                              38.23 s         1000000
----------------------------------------------------------------------
```

### Throughput

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

### Summary

Pool dominates wherever bulk patterns or high-frequency fixed-size allocation is involved.
Single allocation (`Pool Allocate`: 3.18 ms vs `Heap Allocate`: 160.17 ms) shows a 50x
advantage — free list pop vs heap search with no fragmentation overhead.

Bulk allocation tells the same story. Batch allocate (`Pool Batch Allocate`: 8.45 ms vs
`Heap Batch Allocate`: 3.92 s) and batch deallocate (`Pool Batch Deallocate`: 60.65 ms vs
`Heap Batch Deallocate`: 3.55 s) show 40–58x advantages because the pool operates
entirely within a single contiguous slab with no system calls.

Throughput benchmarks reinforce this — fill/drain cycles (`Pool Fill Drain`: 1.05 s vs
`Heap Fill Drain`: 37.43 s) show a 35x advantage, and interleaved alloc/dealloc
(`Pool Interleaved`: 3.18 ms vs `Heap Interleaved`: 124.32 ms) show a 39x advantage.

Reset is O(n) for the pool (relinking all blocks) vs O(n) individual heap deletes.
Pool Reset (553 ms) beats Heap Reset (38.23 s) by 69x — the same asymptotic complexity
but with dramatically lower constant factors from cache locality and no system calls.

Heap wins on object lifecycle with trivially constructible types. `Pool Create`
(4.16 ms) vs `Heap Create` (526 us) favors heap because `Block` has no constructor
overhead, so the pool gains nothing from avoiding heap search while still paying
the `create` wrapper cost.

| Category              | Winner | Notes                                              |
| --------------------- | ------ | -------------------------------------------------- |
| Single allocate       | Pool   | 50x faster — free list pop vs heap search          |
| Alloc/dealloc cycle   | Pool   | 31x faster — no fragmentation overhead             |
| Batch allocate        | Pool   | 464x faster — contiguous slab vs N heap calls      |
| Batch deallocate      | Pool   | 58x faster — free list push vs N heap frees        |
| Fill/drain throughput | Pool   | 35x faster — slab locality vs heap fragmentation   |
| Interleaved           | Pool   | 39x faster — O(1) push/pop vs heap overhead        |
| Reset                 | Pool   | 69x faster — slab relink vs N individual deletes   |
| Object lifecycle      | Heap   | Trivial types gain nothing from pool create/destroy|
| Construction          | Heap   | Slab allocation cost exceeds heap for small pools  |

**Use Pool when:** objects are fixed-size, allocation frequency is high, or bulk patterns are needed.
**Use heap when:** object sizes vary, lifetimes are independent, or object count is very small.

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

## Build Instructions

### Requirements

- GCC 16+ or Clang with C++26 support
- CMake 3.20+

### Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### Run Tests

```bash
./tests
```

### Run Benchmarks

```bash
./benchmarks
```

### Run Examples

```bash
./example_basic
./example_lifecycle
./example_stats
```

---

## Notes

- Blocks must be at least `sizeof(void*)` bytes — enforced via `AP_PRE` contract in `Pool.tpp`.
- `reset()` does not call destructors. Caller is responsible for destroying live objects before reset.
- `reset()` clears all statistics, including `peakUsed_`, when `EnableStats = true`.
- Double-free is undefined behaviour. `deallocate()` rejects foreign pointers via `owns()` but does not detect double-free of valid blocks.
- `getStats()` is only callable on `Pool<true>` — calling it on `Pool<false>` is a compile error.
- `create<T>()` enforces via `AP_PRE` that `sizeof(T) <= blockSize_` and `alignof(T) <= alignment_`.
- `owns()` and the introspection accessors are annotated `AP_PURE` for compiler optimization hints.

---

## License

[MIT](LICENSE) — free to use, modify, and distribute for educational and personal purposes.
