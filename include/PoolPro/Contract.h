/**
 * @file            Contract.h
 * @date            2026-29-7
 *
 * @version         1.0.0
 *
 * @copyright       Copyright (c) 2026 privateMwb
 *                  All rights reserved.
 *                  https://github.com/privateMwb/PoolPro
 *
 * @attention       This source is released under the MIT license
 *                  SPDX-License-Identifier: MIT
 *                  <http://opensource.org/licenses/MIT>
 */

#pragma once

#include <cassert>

// clang-format off

/// @brief Precondition a caller must satisfy before calling a function.
/// Maps to `assert()`; a no-op when `NDEBUG` is defined.
#define AP_PRE(condition)        assert(condition)

/// @brief Postcondition a function guarantees on return.
/// Maps to `assert()`; a no-op when `NDEBUG` is defined.
#define AP_POST(condition)       assert(condition)

/// @brief Condition that must always hold for an object's state.
/// Maps to `assert()`; a no-op when `NDEBUG` is defined.
#define AP_INVARIANT(condition)  assert(condition)

/// @brief Internal implementation assertion, used for invariants that are
/// cheap enough to check unconditionally in debug builds (e.g. detecting
/// an immediate double-free). Maps to `assert()`; a no-op when `NDEBUG`
/// is defined, so it never costs anything in release builds.
#define AP_ASSERT(condition)     assert(condition)

// clang-format on

#if defined(__GNUC__) || defined(__clang__)
/// @brief Hints that a function has no observable side effects and its
/// result depends only on its arguments and/or object state.
#define AP_PURE __attribute__((pure))

/// @brief Software-prefetches `ptr` for a subsequent read, with low
/// temporal locality (data used once, not kept around). Safe to call
/// with an invalid or null address — never faults, on any target
/// GCC/Clang support.
#define AP_PREFETCH(ptr) __builtin_prefetch(static_cast<const void*>(ptr), 0, 1)
#else
#define AP_PURE
#define AP_PREFETCH(ptr) ((void)0)
#endif
