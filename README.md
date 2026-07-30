# PoolAllocator

<p align="center">
  <img src="https://img.shields.io/github/v/release/privateMwb/PoolAllocator?style=for-the-badge&logo=github&color=yellow" alt="Version">
  <img src="https://img.shields.io/badge/License-MIT-orange?style=for-the-badge" alt="License - MIT">
  <img src="https://img.shields.io/badge/C%2B%2B-20-blue?style=for-the-badge&logo=c%2B%2B" alt="C++ - 20">
</p>

<p align="center">
  <a href="https://github.com/privateMwb/PoolAllocator/actions/workflows/build.yml">
    <img src="https://github.com/privateMwb/PoolAllocator/actions/workflows/build.yml/badge.svg" alt="Build and Test">
  </a>
  <a href="https://github.com/privateMwb/PoolAllocator/actions/workflows/benchmark.yml">
    <img src="https://github.com/privateMwb/PoolAllocator/actions/workflows/benchmark.yml/badge.svg" alt="Benchmarks">
  </a>
  <a href="https://github.com/privateMwb/PoolAllocator/actions/workflows/coverage.yml">
    <img src="https://github.com/privateMwb/PoolAllocator/actions/workflows/coverage.yml/badge.svg" alt="Coverage">
  </a>
  <a href="https://github.com/privateMwb/PoolAllocator/actions/workflows/sanitizers.yml">
    <img src="https://github.com/privateMwb/PoolAllocator/actions/workflows/sanitizers.yml/badge.svg" alt="Sanitizers">
  </a>
  <a href="https://github.com/privateMwb/PoolAllocator/actions/workflows/clang-tidy.yml">
    <img src="https://github.com/privateMwb/PoolAllocator/actions/workflows/clang-tidy.yml/badge.svg" alt="Clang Tidy">
  </a>
  <a href="https://github.com/privateMwb/PoolAllocator/actions/workflows/clang-format.yml">
    <img src="https://github.com/privateMwb/PoolAllocator/actions/workflows/clang-format.yml/badge.svg" alt="Clang Format">
  </a>
  <a href="https://github.com/privateMwb/PoolAllocator/actions/workflows/docs.yml">
    <img src="https://github.com/privateMwb/PoolAllocator/actions/workflows/docs.yml/badge.svg" alt="Documentation">
  </a>
  <a href="https://github.com/privateMwb/PoolAllocator/actions/workflows/release.yml">
    <img src="https://github.com/privateMwb/PoolAllocator/actions/workflows/release.yml/badge.svg" alt="Release">
  </a>
  <a href="https://github.com/privateMwb/PoolAllocator/actions/workflows/packaging.yml">
    <img src="https://github.com/privateMwb/PoolAllocator/actions/workflows/packaging.yml/badge.svg" alt="Packaging">
  </a>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/GCC-support-B46F1B?style=flat&logo=gnu" alt="GCC - support">
  <img src="https://img.shields.io/badge/Clang-support-045891?style=flat&logo=llvm" alt="Clang - support">
  <img src="https://img.shields.io/badge/MSVC-support-5C2D91?style=flat" alt="MSVC - support">
  <img src="https://img.shields.io/badge/AppleClang-support-000000?style=flat&logo=apple" alt="AppleClang - support">
</p>

PoolAllocator is a header-only, fixed-capacity, fixed-block-size memory pool for modern C++ — O(1) `allocate()`/`create()` via an intrusive free list backed by a bump-pointer watermark, a single buffer allocated once at construction instead of per-block heap traffic, and batch operations (`allocateBatch()`/`deallocateBatch()`) for handling many blocks in a single call instead of one at a time.

## 📑 Table of Contents

- [Features](#features)
- [Requirements](#requirements)
- [Installation](#installation)
- [Quick Start](#quick-start)
- [Project Structure](#project-structure)
- [Development](#development)
- [Benchmarks](#benchmarks)
- [Documentation](#documentation)
- [Contributing](#contributing)
- [Changelog](#changelog)
- [License](#license)

## <a id="features"></a>✨ Features

- **Single upfront buffer, free-list block reuse** — the whole pool is allocated once at construction; `allocate()`/`deallocate()` reuse blocks via an O(1) intrusive free list, falling back to a bump-pointer watermark only once the free list is empty, so steady-state use never touches the heap again.
- **Batch operations** — `allocateBatch()`/`deallocateBatch()` hand out or reclaim many blocks in a single call, draining the free list before advancing the watermark, instead of paying per-call overhead one block at a time.
- **In-place construction and destruction** — `create<T>()` forwards its arguments directly into `T`'s constructor inside a pool block; `destroy<T>()` runs `T`'s destructor and returns the block to the pool via `deallocate()`.
- **Alignment-aware, contract-based API** — `allocate()` honors the pool's configured alignment, including alignments larger than a block's natural size, and preconditions across the API are documented and enforced via assert-based contracts, consistent everywhere rather than mixed error-handling styles.
- **Optional, zero-cost statistics** — a compile-time `EnableStats` flag adds allocation/usage tracking (`getStats()`) with zero overhead when disabled.

## <a id="requirements"></a>📋 Requirements

- A C++20-conformant compiler (tested: GCC, Clang, MSVC, AppleClang)
- CMake 3.20+

## <a id="installation"></a>📦 Installation

**From source:**

```bash
git clone https://github.com/privateMwb/PoolAllocator.git
cd PoolAllocator
cmake -B build \
  -DBUILD_TESTS=OFF \
  -DBUILD_BENCHMARKS=OFF \
  -DBUILD_REGRESSION=OFF \
  -DBUILD_EXAMPLES=OFF
cmake --install build
```

Then, in your own `CMakeLists.txt`:

```cmake
find_package(PoolPro CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE PoolPro::PoolPro)
```

> vcpkg and Conan packages are built and verified (recipe in
> `packaging/recipes/poolpro/`, port in `packaging/vcpkg/ports/poolpro/`),
> but not yet published to the public registries. This section will be
> updated once they are.

## <a id="quick-start"></a>🚀 Quick Start

```cpp
#include <PoolPro/Pool.h>

int main() {
    PoolPro::Pool<> pool(sizeof(Widget), 64); // 64 fixed-size blocks

    void* raw = pool.allocate();                // raw block
    auto* widget = pool.create<Widget>(1, 2);    // constructed in place

    pool.destroy(widget); // runs ~Widget(); block is returned to the pool
}
```

Handling many blocks at once instead of one at a time:

```cpp
#include <PoolPro/Pool.h>

void processBatch(PoolPro::Pool<>& pool) {
    void* blocks[16];
    std::span<void*> span(blocks, 16);

    std::size_t got = pool.allocateBatch(span); // free-listed blocks first, then virgin memory
    // ... use blocks[0, got) ...
    pool.deallocateBatch(span.subspan(0, got)); // returned to the pool in one call
}
```

Tracking usage with statistics enabled:

```cpp
PoolPro::Pool<true> pool(sizeof(Widget), 256); // EnableStats = true

pool.allocate();

const auto& stats = pool.getStats();
std::cout << stats.allocations_ << " allocations, "
          << stats.peakUsed_ << " blocks at peak\n";
```

## <a id="project-structure"></a>🗂️ Project Structure

```
PoolAllocator/
├── include/
│   └── PoolPro/
│       ├── Pool.h
│       ├── Pool.tpp
│       └── Contract.h
│
├── tests/
│   ├── support/
│   ├── suite/
│   ├── test_main.cpp
│   └── CMakeLists.txt
│
├── benchmarks/
│   ├── support/
│   ├── suite/
│   ├── baselines/
│   ├── bench_main.cpp
│   └── CMakeLists.txt
│
├── examples/
│   ├── support/
│   ├── suite/
│   ├── example_main.cpp
│   └── CMakeLists.txt
│
├── regression/
│   ├── support/
│   ├── regression_main.cpp
│   └── CMakeLists.txt
│
├── packaging/
│   ├── README.md
│   ├── recipes/
│   │   └── poolpro/
│   ├── vcpkg/
│   │   └── ports/
│   │       └── poolpro/
│   └── vcpkg-smoke-test/
│
├── scripts/
│   └── update_package_files.py
│
├── .github/
│   ├── releases/
│   └── workflows/
│
├── cmake/
│   └── PoolProConfig.cmake.in
│
├── docs/
│   ├── Doxyfile
│   └── README.md
│
├── .gitignore
├── CMakeLists.txt
├── README.md
└── LICENSE
```

## <a id="development"></a>🛠️ Development

The from-source install above builds the library only. To work on
PoolPro itself — running tests, benchmarks, or the regression tool —
build with everything enabled (the default):

```bash
cmake -B build
cmake --build build
```

**Run the test suite:**

```bash
ctest --test-dir build
```

**Run benchmarks and check for regressions:**

```bash
./build/benchmarks
./build/regression                  # latest baseline vs. benchmarks/results/benchmark_results.json
./build/regression v1.2.0           # a specific baseline vs. current
./build/regression v1.2.0 v1.4.0    # two baselines against each other
```

`regression` picks the latest baseline by semantic version (`v1.10.0`
correctly outranks `v1.9.0`), not alphabetical filename order, and
auto-names its output (`regression_v1.2.0_vs_current.md`/`.json`, etc.).

See [packaging/README.md](packaging/README.md) for notes on verifying the vcpkg
port and Conan recipe locally.

## <a id="benchmarks"></a>📊 Benchmarks

Measured against `stdPool` (a naive `new`/`delete`-per-block baseline),
same build, at 10K / 100K / 1M iterations (`benchmarks/baselines/v1.0.0.json`
has the full dataset).

| Operation | PoolPro (1M) | stdPool (1M) | Δ |
|---|---|---|---|
| `Allocate() At Capacity` | 2.19 ms | 2.50 s | +114440.3% |
| `Allocate() Large` | 330.34 us | 18.01 ms | +5351.5% |
| `Allocate() 4-byte Alignment` | 308.91 us | 12.12 ms | +3823.7% |
| `Reset() + Refill` | 345.55 ms | 11.94 s | +3354.0% |
| `Allocate() 10M-block Pool` | 395.01 us | 11.73 ms | +2869.7% |
| `Allocate() Room To Spare` | 663.60 us | 12.08 ms | +1720.4% |
| `Reset()` | 331.58 us | 5.55 ms | +1573.5% |
| `Create<T>() Trivial` | 1.07 ms | 15.14 ms | +1312.3% |
| `Deallocate()` | 1.73 ms | 16.07 ms | +826.8% |
| `Destroy<T>() Non-trivial` | 2.24 ms | 17.24 ms | +671.0% |
| `Construction` | 41.04 ms | 149.16 ms | +263.4% |

PoolPro's free-list-backed, fixed-block design pays off most
dramatically on the exhaustion path (a bounds check and a `nullptr`
return versus `stdPool`'s per-call heap traffic), bulk churn (`Reset()`
+ refill), and every allocate/create/destroy/deallocate path, where
`stdPool`'s per-block `new`/`delete` overhead shows up directly.

Unlike a bump-pointer arena, PoolPro shows no measured regression
against its baseline in this suite — even `Construction`, where eager
upfront allocation usually costs the most, still comes out ahead,
since `stdPool` pays for a burst of per-block heap allocations on
first use where PoolPro front-loads a single one. The real trade-off
here is functional rather than measured: a pool is fixed-capacity and
fixed-block-size by design, trading the flexibility of a
general-purpose allocator for the O(1) reuse these numbers are
measuring.

## <a id="documentation"></a>📖 Documentation

Full API reference, generated with Doxygen from `docs/Doxyfile`:

**https://privateMwb.github.io/PoolAllocator/**

## <a id="contributing"></a>🤝 Contributing

Issues and pull requests are welcome. Before submitting a PR:

- Run the test suite (`ctest --test-dir build`)
- If you're changing a hot path, run `./build/regression` and mention
  the results in your PR description

## <a id="changelog"></a>📝 Changelog

See the [Releases](https://github.com/privateMwb/PoolAllocator/releases)
page for version history and release notes.

## <a id="license"></a>📄 License

MIT — see [LICENSE](LICENSE) for details.
