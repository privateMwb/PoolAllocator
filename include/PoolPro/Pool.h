/**
 * @file            Pool.h
 * @date            2026-29-7
 *
 * @version         1.0.0
 *
 * @copyright       Copyright (c) 2026 MWB
 *                  All rights reserved.
 *                  https://github.com/privateMwb/PoolPro
 *
 * @attention       This source is released under the MIT license
 *                  SPDX-License-Identifier: MIT
 *                  <http://opensource.org/licenses/MIT>
 */

#pragma once

// clang-format off
#include <PoolPro/Contract.h> // AP_PRE, AP_PURE, AP_ASSERT, AP_PREFETCH

#include <concepts>    // std::constructible_from
#include <cstddef>     // std::size_t, std::byte, std::max_align_t
#include <limits>      // std::numeric_limits
#include <new>         // ::operator new/delete, std::align_val_t
#include <span>        // std::span
#include <type_traits> // std::conditional_t, std::is_nothrow_constructible_v
#include <utility>     // std::exchange, std::forward
// clang-format on

namespace PoolPro {

/**
 * @brief A fixed-capacity pool allocator with O(1) allocate/deallocate/reset.
 * @tparam EnableStats When true, the pool tracks allocation counters and
 * peak usage via `Stats`/`getStats()`, at the cost of a few extra stores per
 * operation. When false, `Stats` storage is zero-size
 * (`[[no_unique_address]]`) and all statistics bookkeeping is compiled out
 * entirely.
 * @details Every block is in exactly one of three states: (1) virgin — part
 * of the contiguous range at or beyond `watermark_`, never linked or
 * written to; (2) on the intrusive free list `freeList_`, threaded through
 * the blocks themselves via an embedded `FreeNode::next`; or (3) currently
 * allocated to a caller. Because the free list is drained before the
 * watermark is ever touched, a pool that is filled once and then cycled
 * (allocate/deallocate repeatedly) never revisits the watermark path again,
 * and `reset()` never re-walks or re-links memory — it is a handful of
 * stores, not a pass over every block.
 */
template <bool EnableStats = false> class Pool {
  public:
    /// @brief Runtime allocation statistics, present only when `EnableStats` is true.
    struct Stats {
        std::size_t totalAllocated_ = 0; ///< Successful `allocate()`/`allocateBatch()` calls
                                         ///< (lifetime, since last `reset()`).
        std::size_t peakUsed_ = 0; ///< Highest number of simultaneously allocated blocks observed.
        std::size_t allocations_ =
            0; ///< Same as `totalAllocated_`; tracked separately for API stability.
        std::size_t deallocations_ =
            0; ///< Total number of successful `deallocate()`/`deallocateBatch()` calls.
    };

  private:
    /// @brief Zero-size placeholder used in place of `Stats` when `EnableStats` is false.
    struct Empty {};

    /**
     * @brief Free-list node overlaid on an available block's own storage.
     * @details Only ever written into a block at the moment it is released
     * back to the pool. Virgin blocks (at or beyond `watermark_`) never
     * have a `FreeNode` written into them, and are never read from either —
     * `allocate()` hands them out by address arithmetic alone.
     */
    struct FreeNode {
        FreeNode* next; ///< Next free block, or `nullptr` at the end of the list.
    };

    // Pool configuration, fixed for the lifetime of the pool.
    std::size_t blockSize_ = 0;  ///< Requested size of each block, in bytes.
    std::size_t stride_ = 0;     ///< `blockSize_` rounded up to `alignment_`; the actual distance
                                 ///< between consecutive blocks.
    std::size_t blockCount_ = 0; ///< Total number of blocks the pool holds.
    std::size_t alignment_ = 0;  ///< Alignment guaranteed for every block.

    // Backing memory: one contiguous allocation of `stride_ * blockCount_` bytes.
    std::byte* memory_ = nullptr; ///< Start of the backing allocation.
    std::byte* memoryEnd_ =
        nullptr; ///< Cached `memory_ + stride_ * blockCount_`, so bounds checks never recompute it.

    // Free-list / watermark state.
    FreeNode* freeList_ = nullptr; ///< Head of the intrusive free list, or `nullptr` if empty.
    std::size_t watermark_ =
        0; ///< Index of the first never-touched block; `[watermark_, blockCount_)` is virgin.
    std::size_t freeBlockCount_ =
        0; ///< Total blocks available for allocation (free-list entries plus virgin blocks).

    // Facts about `stride_`, cached once at construction.
    bool strideIsPow2_ = false;  ///< Whether `stride_` is itself a power of two.
    std::size_t strideMask_ = 0; ///< `stride_ - 1`; meaningful only when `strideIsPow2_` is true,
                                 ///< letting `owns()` use a mask instead of `%`.

    /// @brief Optional statistics storage; zero-size and fully compiled out when `EnableStats` is
    /// false.
    [[no_unique_address]] std::conditional_t<EnableStats, Stats, Empty> stats_;

  public:
    /**
     * @brief Constructs a pool of `blockCount` blocks, each at least
     * `blockSize` bytes and aligned to `alignment`.
     * @param blockSize Minimum usable size of each block, in bytes. Must be
     * at least `sizeof(void*)`, since a free block must be able to hold a
     * `FreeNode`.
     * @param blockCount Number of blocks to allocate. Must be greater than 0.
     * @param alignment Alignment guaranteed for every block. Must be a
     * power of two. Defaults to `alignof(std::max_align_t)`.
     * @throws std::bad_alloc if the backing allocation fails.
     * @details Makes a single `stride * blockCount`-byte allocation and
     * leaves every block virgin. No per-block initialization happens here
     * — construction cost does not scale with `blockCount` beyond the one
     * allocation call.
     */
    /// @throws Nothing itself, but preconditions are checked (via `AP_PRE`,
    /// i.e. `assert()`) strictly before the backing allocation is
    /// attempted, so a violated precondition never reaches `::operator
    /// new` with garbage arguments — even in the member-initializer list.
    explicit Pool(std::size_t blockSize, std::size_t blockCount,
                  std::size_t alignment = alignof(std::max_align_t));

    /// @brief Releases the backing allocation. Does not destroy any objects
    /// still live in allocated blocks — callers must `destroy()` them first.
    ~Pool() noexcept;

    Pool(const Pool&) = delete;
    Pool& operator=(const Pool&) = delete;

    /**
     * @brief Move-constructs a pool, taking ownership of `other`'s backing
     * allocation and free-list state.
     * @param other Pool to move from. Left in a valid, empty (zero-capacity) state.
     */
    Pool(Pool&& other) noexcept;

    /**
     * @brief Move-assigns from `other`, releasing this pool's current
     * allocation first.
     * @param other Pool to move from. Left in a valid, empty (zero-capacity) state.
     * @return Reference to `*this`.
     */
    Pool& operator=(Pool&& other) noexcept;

    /**
     * @brief Allocates one block.
     * @return Pointer to an uninitialized block of at least `blockSize`
     * bytes aligned to `alignment`, or `nullptr` if the pool is exhausted.
     * @details Reuses the most recently freed block if one is available,
     * otherwise bumps the watermark into virgin memory. Writes no
     * bookkeeping into the returned block.
     */
    [[nodiscard]] void* allocate() noexcept;

    /**
     * @brief Returns a previously allocated block to the pool.
     * @param ptr Pointer previously returned by `allocate()`/
     * `allocateBatch()`/`create()` on this pool. `nullptr` and pointers not
     * owned by this pool are silently ignored.
     * @details Does not destroy any object at `ptr` — for owning types,
     * call `destroy()` instead.
     */
    void deallocate(void* ptr) noexcept;

    /**
     * @brief Allocates up to `out.size()` blocks in one call.
     * @param out Destination for the allocated block pointers.
     * @return Number of blocks actually allocated; may be fewer than
     * `out.size()` if the pool runs out.
     * @details Drains the free list first, then bump-allocates any
     * remainder directly from virgin memory in a branch-free loop with no
     * dependent loads. Statistics (when enabled) are updated once for the
     * whole batch rather than once per block.
     */
    [[nodiscard]] std::size_t allocateBatch(std::span<void*> out) noexcept;

    /**
     * @brief Returns a batch of previously allocated blocks to the pool.
     * @param ptrs Pointers to return. `nullptr` and pointers not owned by
     * this pool are silently skipped.
     * @details Builds the updated free list in a local variable and writes
     * `freeList_` back once at the end, rather than once per pointer.
     */
    void deallocateBatch(std::span<void*> ptrs) noexcept;

    /**
     * @brief Allocates a block and constructs a `T` in it.
     * @tparam T Type to construct. Must satisfy `sizeof(T) <= blockSize`
     * and `alignof(T) <= alignment` (checked via `AP_PRE`).
     * @tparam Args Deduced constructor argument types.
     * @param args Forwarded to `T`'s constructor.
     * @return Pointer to the constructed `T`, or `nullptr` if the pool is exhausted.
     * @throws Whatever `T`'s constructor throws. If it throws, the block is
     * returned to the pool before the exception propagates, so no block is
     * ever lost. This handling is compiled out entirely when `T` is
     * nothrow-constructible.
     */
    template <typename T, typename... Args>
        requires std::constructible_from<T, Args...>
    [[nodiscard]] T* create(Args&&... args);

    /**
     * @brief Destroys a `T` previously constructed with `create()` and
     * returns its block to the pool.
     * @tparam T Type to destroy.
     * @param ptr Pointer previously returned by `create<T>()` on this pool.
     * `nullptr` and pointers not owned by this pool are silently ignored.
     */
    template <typename T> void destroy(T* ptr) noexcept;

    /**
     * @brief Restores the pool to its initial, fully-available state.
     * @details Clears the free list and resets the watermark to 0 — O(1)
     * regardless of `blockCount`. Does not destroy any objects still live
     * in previously allocated blocks; callers must destroy them first if
     * they have non-trivial destructors.
     */
    void reset() noexcept;

    /**
     * @brief Checks whether `ptr` is a valid block address owned by this pool.
     * @param ptr Address to check.
     * @return `true` if `ptr` points at the start of one of this pool's
     * blocks (whether currently allocated or free).
     */
    [[nodiscard]] AP_PURE bool owns(const void* ptr) const noexcept;

    /// @brief Returns runtime allocation statistics. Only available when `EnableStats` is true.
    [[nodiscard]] const Stats& getStats() const noexcept
        requires EnableStats;

    /// @brief Returns the total backing capacity in bytes (`blockStride() * totalBlocks()`).
    [[nodiscard]] AP_PURE std::size_t capacity() const noexcept;
    /// @brief Returns the number of blocks currently allocated.
    [[nodiscard]] AP_PURE std::size_t usedBlocks() const noexcept;
    /// @brief Returns the number of blocks currently available for allocation.
    [[nodiscard]] AP_PURE std::size_t freeBlocks() const noexcept;
    /// @brief Returns the total number of blocks the pool holds.
    [[nodiscard]] AP_PURE std::size_t totalBlocks() const noexcept;
    /// @brief Returns the stride between consecutive blocks, in bytes (`blockSize` rounded up to
    /// `alignment`).
    [[nodiscard]] AP_PURE std::size_t blockStride() const noexcept;

  private:
    /// @brief Returns whether `value` is a power of two (`0` is not).
    [[nodiscard]] static constexpr bool isPowerOfTwo(std::size_t value) noexcept;

    /**
     * @brief Rounds `value` up to the nearest multiple of `alignment`.
     * @param value Value to round up.
     * @param alignment Alignment to round to. Must be a power of two.
     * @return `value` rounded up to `alignment`.
     */
    [[nodiscard]] static constexpr std::size_t alignForward(std::size_t value,
                                                            std::size_t alignment) noexcept;

    /**
     * @brief Validates constructor preconditions and computes `stride_`.
     * @details Runs `AP_PRE(blockCount > 0)`, `AP_PRE(isPowerOfTwo(alignment))`,
     * and `AP_PRE(blockSize >= sizeof(FreeNode))` before doing anything
     * else. Because `stride_` is initialized before `memory_` in the
     * member-initializer list, these checks are guaranteed to run before
     * the backing allocation is ever attempted — a violated precondition
     * (e.g. `alignment == 0`) is caught here instead of reaching
     * `::operator new`/`alignForward` with garbage arguments.
     */
    [[nodiscard]] static constexpr std::size_t
    validateAndComputeStride(std::size_t blockSize, std::size_t blockCount,
                             std::size_t alignment) noexcept;

    /**
     * @brief Performs the backing allocation for `stride * blockCount` bytes.
     * @details Asserts (debug builds only) that `stride * blockCount` does
     * not overflow `std::size_t` before multiplying, so a pathologically
     * large `blockCount` is caught rather than silently wrapping into an
     * undersized allocation.
     * @throws std::bad_alloc if the allocation fails.
     */
    [[nodiscard]] static std::byte* allocateStorage(std::size_t stride, std::size_t blockCount,
                                                    std::size_t alignment);

    /**
     * @brief Takes one block off the pool, preferring the free list.
     * @return Pointer to an available block, or `nullptr` if none remain.
     * @details The free-list path pops `freeList_` and, on GCC/Clang,
     * prefetches the node it now points to, so the *next* call is less
     * likely to stall on a cache miss walking the list. The watermark path
     * is pure pointer arithmetic — it never reads the block it returns,
     * since a virgin block holds no linked state to read.
     */
    [[nodiscard]] void* acquireBlock() noexcept;

    /**
     * @brief Returns one block to the free list.
     * @param ptr Block to release. Caller must have already validated ownership.
     * @details Asserts (debug builds only) that `ptr` isn't already the
     * current free-list head, catching the most common double-free pattern
     * — freeing the same pointer twice in a row — at zero cost in release
     * builds.
     */
    void releaseBlock(void* ptr) noexcept;

    /// @brief Clears the free list and resets the watermark; used by both
    /// the constructor and `reset()`.
    void initializeFreeList() noexcept;

    /// @brief Updates allocation statistics for `count` successful allocations, given the resulting
    /// used-block count.
    constexpr void statAlloc(std::size_t usedNow, std::size_t count = 1) noexcept;
    /// @brief Updates deallocation statistics for `count` successful deallocations.
    constexpr void statDealloc(std::size_t count = 1) noexcept;
};

} // namespace PoolPro

/// @brief Umbrella alias so this library's types are reachable as
/// `rain::Pool`, alongside every other project library, while its true
/// namespace (and all internal diagnostics) remains `PoolPro`. Reopens
/// `rain` rather than aliasing it, since multiple libraries each contribute
/// their own names into the same `rain` namespace -- an alias
/// (`namespace rain = PoolPro;`) can only ever bind to one target and
/// collides the moment a second library declares its own `rain` alias to
/// something else. Declared here only: Pool.tpp is included directly by
/// this header (not a separate entry point), and Contract.h is included
/// directly by this header too, so both are already reachable through it.
namespace rain {
using namespace PoolPro;
}

#include "Pool.tpp"
