#pragma once

#include <cassert>

// ============================================================================
// AllocatorPro Contract Macros
//
// These macros document function contracts and internal assumptions.
//
// AP_PRE       - Preconditions that callers must satisfy.
// AP_POST      - Postconditions guaranteed by the function.
// AP_INVARIANT - Conditions that must always hold for an object's state.
// AP_ASSERT    - Internal implementation assertions.
//
// By default, all contract macros map to assert(). Their implementation can
// be replaced globally without modifying library source code.
//
// Compiler Attributes
//
// AP_PURE      - Indicates that a function has no observable side effects and
//                its return value depends only on its arguments and/or object
//                state. Enables additional compiler optimizations when
//                supported.
//
// By default, all contract macros map to assert(). Their implementation can
// be replaced globally without modifying library source code.
//
// In release builds (NDEBUG defined), all contract macros expand to no-ops.
// Callers are responsible for satisfying preconditions unconditionally.
//
// Compiler Attributes
// ============================================================================

// Contract macros.
#define AP_PRE(condition)        assert(condition)
#define AP_POST(condition)       assert(condition)
#define AP_INVARIANT(condition)  assert(condition)
#define AP_ASSERT(condition)     assert(condition)

// Compiler attributes.
#if defined(__GNUC__) || defined(__clang__)
    #define AP_PURE __attribute__((pure))
#else
    #define AP_PURE
#endif
