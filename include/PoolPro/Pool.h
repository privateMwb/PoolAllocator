#pragma once

#include <PoolPro/Contract.h>

#include <concepts>
#include <cstddef>
#include <new>
#include <span>
#include <type_traits>

namespace AllocatorPro {

// A fixed-size pool allocator.
// Provides constant-time allocation and deallocation for blocks of
// uniform size using an intrusive free list.
template<bool EnableStats = false>
class Pool {
public:

	// Runtime allocation statistics.
	// Present only when EnableStats is true.
	struct Stats {
		std::size_t totalAllocated_ = 0;
		std::size_t peakUsed_       = 0;
		std::size_t allocations_    = 0;
		std::size_t deallocations_  = 0;
	};

private:

	// Zero-size placeholder used when statistics are disabled.
	struct Empty {};

	// Free list node embedded within available blocks.
	struct FreeNode {
		FreeNode* next;
	};

	// Pool configuration.
	std::size_t blockSize_   = 0;
	std::size_t stride_      = 0;
	std::size_t blockCount_  = 0;
	std::size_t alignment_   = 0;

	// Backing memory.
	std::byte* memory_ = nullptr;

	// Free list state.
	FreeNode*   freeList_       = nullptr;
	std::size_t freeBlockCount_ = 0;

	// Optional statistics storage with zero runtime overhead when disabled.
	[[no_unique_address]]
	std::conditional_t<EnableStats, Stats, Empty> stats_;

public:

	// Constructors and destructor.
	explicit Pool(std::size_t blockSize,
	              std::size_t blockCount,
	              std::size_t alignment = alignof(std::max_align_t));

	~Pool() noexcept;

	Pool(const Pool&)            = delete;
	Pool& operator=(const Pool&) = delete;

	Pool(Pool&& other)            noexcept;
	Pool& operator=(Pool&& other) noexcept;

	// Memory allocation utilities.
	[[nodiscard]] void* allocate() noexcept;
	void deallocate(void* ptr) noexcept;

	[[nodiscard]] std::size_t allocateBatch(std::span<void*> out) noexcept;
	void deallocateBatch(std::span<void*> ptrs) noexcept;

	// Object construction and destruction utilities.
	template<typename T, typename... Args>
	requires std::constructible_from<T, Args...>
	[[nodiscard]] T* create(Args&&... args);

	template<typename T>
	void destroy(T* ptr) noexcept;

	// Restores the pool to its initial state.
	void reset() noexcept;

	// Returns whether the pointer belongs to this pool.
	[[nodiscard]] AP_PURE bool owns(const void* ptr) const noexcept;

	// Returns runtime allocation statistics.
	[[nodiscard]] const Stats& getStats() const noexcept
	requires EnableStats;

	// Returns pool usage information.
	[[nodiscard]] AP_PURE std::size_t capacity()    const noexcept;
	[[nodiscard]] AP_PURE std::size_t usedBlocks()  const noexcept;
	[[nodiscard]] AP_PURE std::size_t freeBlocks()  const noexcept;
	[[nodiscard]] AP_PURE std::size_t totalBlocks() const noexcept;
	[[nodiscard]] AP_PURE std::size_t blockStride() const noexcept;

private:

	// Memory alignment helper utilities.
	[[nodiscard]] static constexpr bool isPowerOfTwo(std::size_t value) noexcept;
	[[nodiscard]] static constexpr std::size_t alignForward(
		std::size_t value,
		std::size_t alignment) noexcept;

	// Internal free list helper.
	void initializeFreeList() noexcept;

	// Internal statistics helpers.
	constexpr void statAlloc(std::size_t usedNow) noexcept;
	constexpr void statDealloc() noexcept;
};

} // namespace AllocatorPro

#include "Pool.tpp"