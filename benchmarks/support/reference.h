#pragma once

#include <memory_resource>

// The standard implementation benchmarked against PoolPro — the
// standard library's own pool allocator.
using stdPool = std::pmr::unsynchronized_pool_resource;