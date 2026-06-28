#pragma once

#include <iostream>
#include <concepts>
#include <cstddef>
#include <new>
#include <span>
#include <utility>
#include <cassert>

namespace AllocatorPro {

template<bool EnableStats = false>
class Pool {
public:

	// Allocation Statistics
	struct Stats {
		std::size_t totalAllocated_ = 0;
		std::size_t peakUsed_       = 0;
		std::size_t allocations_    = 0;
		std::size_t deallocations_  = 0;
	};

private:

	// Free List Node
	struct FreeNode {
		FreeNode* next;
	};

	// Empty Statistics Type
	struct Empty {};

	// Pool Configuration
	std::size_t blockSize_   = 0;
	std::size_t stride_      = 0;
	std::size_t blockCount_  = 0;
	std::size_t alignment_   = 0;

    // Pool Memory
	std::byte* memory_ = nullptr;
	
	// Allocation State
	FreeNode*   freeList_        = nullptr;
	std::size_t freeBlockCount_  = 0;

	// Statistics Storage
	[[no_unique_address]]
	std::conditional_t<EnableStats, Stats, Empty> stats_;

public:

	// Constructors & Destructor
	explicit Pool(std::size_t blockSize,
	              std::size_t blockCount,
	              std::size_t alignment = alignof(std::max_align_t));

	~Pool() noexcept;

	Pool(const Pool&)             = delete;
	Pool& operator=(const Pool&)  = delete;

	Pool(Pool&& other)             noexcept;
	Pool& operator=(Pool&& other)  noexcept;

	// Memory Allocation
	[[nodiscard]] void* allocate()  noexcept;
	void deallocate(void* ptr)      noexcept;

	[[nodiscard]] std::size_t allocateBatch(std::span<void*> out)  noexcept;
	void deallocateBatch(std::span<void*> ptrs)                    noexcept;

	// Object Lifecycle
	template<typename T, typename... Args>
	requires std::constructible_from<T, Args...>
	[[nodiscard]] T* create(Args&&... args);

	template<typename T>
	void destroy(T* ptr) noexcept;

	// Pool Management
	void reset() noexcept;

	// Introspection
	[[nodiscard]] bool owns(const void* ptr) const noexcept;

	[[nodiscard]] const Stats& getStats() const noexcept
	requires EnableStats;

	[[nodiscard]] std::size_t capacity()     const noexcept;
	[[nodiscard]] std::size_t usedBlocks()   const noexcept;
	[[nodiscard]] std::size_t freeBlocks()   const noexcept;
	[[nodiscard]] std::size_t totalBlocks()  const noexcept;
	[[nodiscard]] std::size_t blockStride()  const noexcept;

private:

	// Utility Functions
	[[nodiscard]] static constexpr bool isPowerOfTwo(std::size_t value) noexcept;
	[[nodiscard]] static constexpr std::size_t alignForward(std::size_t value,
	        std::size_t alignment) noexcept;

	// Free List Helpers
	void initializeFreeList() noexcept;

	// Statistics Helpers
	constexpr void statAlloc(std::size_t usedNow)  noexcept;
	constexpr void statDealloc()                   noexcept;
};

} // namespace AllocatorPro

#include "Pool.tpp"