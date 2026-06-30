cmake_minimum_required(VERSION 3.20)
project(
    PoolPro
    VERSION 1.0.0
    LANGUAGES CXX
)


# ============================================================================
# C++ Configuration
# ============================================================================

# Configure the C++ language standard.
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# ============================================================================
# PoolPro (Header-only Library)
# ============================================================================

# Define the header-only PoolPro library.
add_library(PoolPro INTERFACE)

# Expose the public include directory to consumers.
target_include_directories(
    PoolPro
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Require C++23 for all targets that consume PoolPro.
target_compile_features(
    PoolPro
    INTERFACE
    cxx_std_23
)

# ============================================================================
# Tests
# ============================================================================

# Build the test executable.
add_executable(
    tests
    tests/test_all.cpp
    tests/test_constructor.cpp
    tests/test_move.cpp
    tests/test_memory.cpp
    tests/test_lifecycle.cpp
    tests/test_state.cpp
    tests/test_introspection.cpp
    tests/test_stats.cpp
    tests/test_edge.cpp
)

# Link PoolPro to the test executable.
target_link_libraries(
    tests
    PRIVATE
    PoolPro
)


# ============================================================================
# Benchmarks
# ============================================================================

# Build the benchmark executable.
add_executable(
    benchmarks
    benchmarks/bench_all.cpp
    benchmarks/bench_constructor.cpp
    benchmarks/bench_allocation.cpp
    benchmarks/bench_bulk.cpp
    benchmarks/bench_lifecycle.cpp
    benchmarks/bench_reset.cpp
    benchmarks/bench_throughput.cpp
)

# Link PoolPro  to the benchmark executable.
target_link_libraries(
    benchmarks
    PRIVATE
    PoolPro
)


# ============================================================================
# Examples
# ============================================================================

# Build the basic usage example.
add_executable(
    example_basic
    examples/example_basic.cpp
)

# Link PoolPro to the basic usage example.
target_link_libraries(
    example_basic
    PRIVATE
    PoolPro
)

# Build the object lifecycle example.
add_executable(
    example_lifecycle
    examples/example_lifecycle.cpp
)

# Link PoolPro to the object lifecycle example.
target_link_libraries(
    example_lifecycle
    PRIVATE
    PoolPro
)

# Build the statistics example.
add_executable(
    example_stats
    examples/example_stats.cpp
)

# Link StackPro to the statistics example.
target_link_libraries(
    example_stats
    PRIVATE
    PoolPro
)


# ============================================================================
# Installation
# ============================================================================

# Install the PoolPro target and export it for downstream projects.
install(
    TARGETS PoolPro
    EXPORT PoolProTargets
)

# Install the exported CMake targets.
install(
    EXPORT PoolProTargets
    FILE PoolProTargets.cmake
    NAMESPACE PoolPro::
    DESTINATION lib/cmake/PoolPro
)

# Install the public header files.
install(
    DIRECTORY include/
    DESTINATION include
)


# ============================================================================
# Packaging
# ============================================================================

include(CMakePackageConfigHelpers)

# Generate the package configuration file.
configure_package_config_file(
    cmake/PoolProConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/PoolProConfig.cmake
    INSTALL_DESTINATION lib/cmake/PoolPro
)

# Generate the package version file.
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/PoolProConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Install the package configuration files.
install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/PoolProConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/PoolProConfigVersion.cmake
    DESTINATION lib/cmake/PoolPro
)
