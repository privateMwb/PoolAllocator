cmake_minimum_required(VERSION 3.20)
project(PoolPro LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Header-only library
add_library(PoolPro INTERFACE)

target_include_directories(PoolPro
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Tests
add_executable(tests
    tests/test_all.cpp
    tests/test_constructor.cpp
    tests/test_memory_allocation.cpp
    tests/test_object_lifecycle.cpp
    tests/test_pool_management.cpp
    tests/test_introspection.cpp
    tests/test_edge_cases.cpp
)

target_link_libraries(tests PRIVATE PoolPro)

# Benchmarks
add_executable(benchmarks
    benchmarks/bench_all.cpp
    benchmarks/bench_constructor.cpp
    benchmarks/bench_allocation.cpp
    benchmarks/bench_bulk_allocation.cpp
    benchmarks/bench_object_lifecycle.cpp
    benchmarks/bench_reset.cpp
    benchmarks/bench_throughput.cpp
)

target_link_libraries(benchmarks PRIVATE PoolPro)

# Examples
add_executable(ex1
    examples/example_basic.cpp
)

target_link_libraries(ex1 PRIVATE PoolPro)

add_executable(ex2
    examples/example_lifecycle.cpp
)

target_link_libraries(ex2 PRIVATE PoolPro)