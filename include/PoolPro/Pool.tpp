// ============================================================
// Pool.tpp
// Template implementation for AllocatorPro::Pool.
// ============================================================
//
//  Sections:
//   1. Constructors & Destructor
//   2. Move Semantics
//   3. Memory Allocation
//   4. Object Lifecycle
//   5. Pool Management
//   6. Introspection
//   7. Utility Functions
//   8. Free List Helpers
//   9. Statistics Helpers
//
// ============================================================

namespace AllocatorPro {

// ============================================================
//  Section 1 — Constructors & Destructor
// ============================================================

template<bool EnableStats>
Pool<EnableStats>::Pool(std::size_t blockSize,
                        std::size_t blockCount,
                        std::size_t alignment)
	: blockSize_(blockSize)
	, stride_(alignForward(blockSize, alignment))
	, blockCount_(blockCount)
	, alignment_(alignment)
	, memory_(static_cast<std::byte*>(
	              ::operator new(
	                  stride_ * blockCount_,
	                  std::align_val_t{alignment})))
	, freeList_(nullptr)
	, freeBlockCount_(blockCount)
	, stats_{} {
	AP_PRE(blockSize >= sizeof(FreeNode));
  AP_PRE(blockCount > 0);
  AP_PRE(isPowerOfTwo(alignment));

	initializeFreeList();
}

template<bool EnableStats>
Pool<EnableStats>::~Pool() noexcept {
	if (memory_)
    ::operator delete(memory_, std::align_val_t{alignment_});
}


// ============================================================
//  Section 2 — Move Semantics
// ============================================================

template<bool EnableStats>
Pool<EnableStats>::Pool(Pool&& other) noexcept
	: memory_(std::exchange(other.memory_, nullptr))
	, blockSize_(std::exchange(other.blockSize_, 0))
	, stride_(std::exchange(other.stride_, 0))
	, blockCount_(std::exchange(other.blockCount_, 0))
	, alignment_(std::exchange(other.alignment_, 0))
	, freeList_(std::exchange(other.freeList_, nullptr))
	, freeBlockCount_(std::exchange(other.freeBlockCount_, 0))
	, stats_(std::exchange(other.stats_, {}))
{}


template<bool EnableStats>
Pool<EnableStats>&
Pool<EnableStats>::operator=(Pool&& other) noexcept {
	if (this != &other) {
		if (memory_)
			::operator delete(memory_, std::align_val_t{alignment_});

		memory_         = std::exchange(other.memory_, nullptr);
		blockSize_      = std::exchange(other.blockSize_, 0);
		stride_         = std::exchange(other.stride_, 0);
		blockCount_     = std::exchange(other.blockCount_, 0);
		alignment_      = std::exchange(other.alignment_, 0);
		freeList_       = std::exchange(other.freeList_, nullptr);
		freeBlockCount_ = std::exchange(other.freeBlockCount_, 0);
		stats_          = std::exchange(other.stats_, {});
	}

	return *this;
}


// ============================================================
//  Section 3 — Memory Allocation
// ============================================================

template<bool EnableStats>
void* Pool<EnableStats>::allocate() noexcept {
	if (freeList_ == nullptr) return nullptr;

	FreeNode* block = freeList_;
	freeList_ = freeList_->next;
	--freeBlockCount_;

	statAlloc(usedBlocks());

	return block;
}

template<bool EnableStats>
void Pool<EnableStats>::deallocate(void* ptr) noexcept {
	if (ptr == nullptr || !owns(ptr)) return;

	auto* node  = static_cast<FreeNode*>(ptr);
	node->next  = freeList_;
	freeList_   = node;
	++freeBlockCount_;

	statDealloc();
}

template<bool EnableStats>
std::size_t Pool<EnableStats>::allocateBatch(std::span<void*> out) noexcept {
	std::size_t count = 0;

	while (count < out.size() && freeList_) {
		FreeNode* block = freeList_;
		freeList_ = block->next;
		--freeBlockCount_;

		out[count++] = block;

		statAlloc(usedBlocks());
	}

	return count;
}

template<bool EnableStats>
void Pool<EnableStats>::deallocateBatch(std::span<void*> ptrs) noexcept {
	for (void* ptr : ptrs) {
		if (!ptr || !owns(ptr)) continue;

		auto* node = static_cast<FreeNode*>(ptr);
		node->next = freeList_;
		freeList_ = node;
		++freeBlockCount_;

		statDealloc();
	}
}


// ============================================================
//  Section 4 — Object Lifecycle
// ============================================================

template<bool EnableStats>
template<typename T, typename... Args>
requires std::constructible_from<T, Args...>
T* Pool<EnableStats>::create(Args&&... args) {
	AP_PRE(sizeof(T) <= blockSize_);
  AP_PRE(alignof(T) <= alignment_);

	void* block = allocate();
	if (!block) return nullptr;

	return ::new (block) T(std::forward<Args>(args)...);
}

template<bool EnableStats>
template<typename T>
void Pool<EnableStats>::destroy(T* ptr) noexcept {
	if (ptr == nullptr || !owns(ptr))
    return;

  ptr->~T();
  deallocate(ptr);
}


// ============================================================
//  Section 5 — Pool Management
// ============================================================

template<bool EnableStats>
void Pool<EnableStats>::reset() noexcept {
	initializeFreeList();

	if constexpr (EnableStats) {
		stats_.totalAllocated_  = 0;
		stats_.allocations_     = 0;
		stats_.deallocations_   = 0;
		stats_.peakUsed_        = 0;
	}
}


// ============================================================
//  Section 6 — Introspection
// ============================================================

template<bool EnableStats>
bool Pool<EnableStats>::owns(const void* ptr) const noexcept {
	const auto* p = static_cast<const std::byte*>(ptr);

	if (p < memory_ || p >= memory_ + stride_ * blockCount_)
		return false;

	return ((p - memory_) % stride_) == 0;
}

template<bool EnableStats>
auto Pool<EnableStats>::getStats() const noexcept -> const Stats&
requires EnableStats {
	return stats_;
}

template<bool EnableStats>
std::size_t Pool<EnableStats>::capacity() const noexcept {
	return stride_ * blockCount_;
}

template<bool EnableStats>
std::size_t Pool<EnableStats>::usedBlocks()   const noexcept {
	return blockCount_ - freeBlockCount_;
}

template<bool EnableStats>
std::size_t Pool<EnableStats>::freeBlocks()   const noexcept {
	return freeBlockCount_;
}

template<bool EnableStats>
std::size_t Pool<EnableStats>::totalBlocks()  const noexcept {
	return blockCount_;
}

template<bool EnableStats>
std::size_t Pool<EnableStats>::blockStride()  const noexcept {
	return stride_;
}


// ============================================================
//  Section 7 — Utility Functions
// ============================================================

template<bool EnableStats>
constexpr bool Pool<EnableStats>::isPowerOfTwo(std::size_t value) noexcept {
	return value != 0 && (value & (value - 1)) == 0;
}

template<bool EnableStats>
constexpr std::size_t
Pool<EnableStats>::alignForward(std::size_t value, std::size_t alignment) noexcept {
    AP_PRE(isPowerOfTwo(alignment));

    return (value + alignment - 1) & ~(alignment - 1);
}


// ============================================================
//  Section 8 — Free List Helpers
// ============================================================

template<bool EnableStats>
void Pool<EnableStats>::initializeFreeList() noexcept {
    AP_ASSERT(memory_ != nullptr);
    AP_ASSERT(blockCount_ > 0);

    freeList_ = reinterpret_cast<FreeNode*>(memory_);
    freeBlockCount_ = blockCount_;
    FreeNode* current = freeList_;

    for (std::size_t i = 1; i < blockCount_; ++i) {
        current->next =
            reinterpret_cast<FreeNode*>(memory_ + i * stride_);
        current = current->next;
    }

    current->next = nullptr;
}


// ============================================================
//  Section 9 — Statistics Helpers
// ============================================================

template<bool EnableStats>
constexpr void Pool<EnableStats>::statAlloc(std::size_t usedNow) noexcept {
	if constexpr (EnableStats) {
		++stats_.totalAllocated_;
		++stats_.allocations_;

		if (usedNow > stats_.peakUsed_)
			stats_.peakUsed_ = usedNow;
	}
}

template<bool EnableStats>
constexpr void Pool<EnableStats>::statDealloc() noexcept {
	if constexpr (EnableStats)
		++stats_.deallocations_;
}

} // namespace AllocatorPro