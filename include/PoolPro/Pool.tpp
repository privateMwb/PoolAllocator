/**
 * @file Pool.tpp
 * @brief Pool template implementation.
 *
 * Contains the implementation of Pool template member functions and
 * internal implementation details.
 */

// ============================================================
// Template implementation for PoolPro::Pool.
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

namespace PoolPro {

// ============================================================
//  Section 1 — Constructors & Destructor
// ============================================================

template <bool EnableStats>
Pool<EnableStats>::Pool(std::size_t blockSize, std::size_t blockCount, std::size_t alignment)
    : blockSize_(blockSize), stride_(validateAndComputeStride(blockSize, blockCount, alignment)),
      blockCount_(blockCount), alignment_(alignment),
      memory_(allocateStorage(stride_, blockCount_, alignment_)),
      memoryEnd_(memory_ + stride_ * blockCount_), freeList_(nullptr), watermark_(0),
      freeBlockCount_(blockCount_), strideIsPow2_(isPowerOfTwo(stride_)),
      strideMask_(strideIsPow2_ ? stride_ - 1 : 0), stats_{} {
    // All preconditions were already checked in validateAndComputeStride()
    // and allocateStorage(), strictly before either of them ran — so by
    // the time this body executes, blockCount_, alignment_, and stride_
    // are all known-good and the allocation has already succeeded.
    //
    // The pool starts fully available with no per-block writes: freeList_
    // is empty and watermark_ is 0, so every block is virgin. This is the
    // same state initializeFreeList() produces, just reached directly in
    // the member-init list instead of via a function call.
}

template <bool EnableStats> Pool<EnableStats>::~Pool() noexcept {
    if (memory_)
        ::operator delete(memory_, std::align_val_t(alignment_));
}

// ============================================================
//  Section 2 — Move Semantics
// ============================================================

template <bool EnableStats>
Pool<EnableStats>::Pool(Pool&& other) noexcept
    : blockSize_(std::exchange(other.blockSize_, 0)), stride_(std::exchange(other.stride_, 0)),
      blockCount_(std::exchange(other.blockCount_, 0)),
      alignment_(std::exchange(other.alignment_, 0)),
      memory_(std::exchange(other.memory_, nullptr)),
      memoryEnd_(std::exchange(other.memoryEnd_, nullptr)),
      freeList_(std::exchange(other.freeList_, nullptr)),
      watermark_(std::exchange(other.watermark_, 0)),
      freeBlockCount_(std::exchange(other.freeBlockCount_, 0)),
      strideIsPow2_(std::exchange(other.strideIsPow2_, false)),
      strideMask_(std::exchange(other.strideMask_, 0)),
      stats_(std::exchange(other.stats_, decltype(stats_){})) {}

template <bool EnableStats> Pool<EnableStats>& Pool<EnableStats>::operator=(Pool&& other) noexcept {
    if (this != &other) {
        if (memory_)
            ::operator delete(memory_, std::align_val_t(alignment_));

        blockSize_ = std::exchange(other.blockSize_, 0);
        stride_ = std::exchange(other.stride_, 0);
        blockCount_ = std::exchange(other.blockCount_, 0);
        alignment_ = std::exchange(other.alignment_, 0);
        memory_ = std::exchange(other.memory_, nullptr);
        memoryEnd_ = std::exchange(other.memoryEnd_, nullptr);
        freeList_ = std::exchange(other.freeList_, nullptr);
        watermark_ = std::exchange(other.watermark_, 0);
        freeBlockCount_ = std::exchange(other.freeBlockCount_, 0);
        strideIsPow2_ = std::exchange(other.strideIsPow2_, false);
        strideMask_ = std::exchange(other.strideMask_, 0);
        stats_ = std::exchange(other.stats_, decltype(stats_){});
    }
    return *this;
}

// ============================================================
//  Section 3 — Memory Allocation
// ============================================================

template <bool EnableStats> void* Pool<EnableStats>::allocate() noexcept {
    void* block = acquireBlock();
    if (block)
        statAlloc(usedBlocks());
    return block;
}

template <bool EnableStats> void Pool<EnableStats>::deallocate(void* ptr) noexcept {
    if (ptr == nullptr || !owns(ptr)) [[unlikely]] {
        return;
    }

    releaseBlock(ptr);
    statDealloc();
}

template <bool EnableStats>
std::size_t Pool<EnableStats>::allocateBatch(std::span<void*> out) noexcept {
    std::size_t count = 0;
    const std::size_t requested = out.size();

    // Drain the free list first: LIFO reuse of recently freed blocks.
    while (count < requested && freeList_) {
        FreeNode* node = freeList_;
        freeList_ = node->next;
        out[count++] = node;
    }

    // Bump-allocate any remainder directly from virgin memory. Pure
    // address arithmetic with no dependent loads, so this loop has no
    // data hazards and is trivially unrollable/vectorizable.
    if (count < requested) {
        const std::size_t available = blockCount_ - watermark_;
        const std::size_t take = (requested - count < available) ? (requested - count) : available;

        std::byte* base = memory_ + watermark_ * stride_;
        for (std::size_t i = 0; i < take; ++i) {
            out[count + i] = base + i * stride_;
        }

        watermark_ += take;
        count += take;
    }

    freeBlockCount_ -= count;
    if (count)
        statAlloc(usedBlocks(), count);

    return count;
}

template <bool EnableStats>
void Pool<EnableStats>::deallocateBatch(std::span<void*> ptrs) noexcept {
    // Build the updated list in a local variable and write freeList_ back
    // once at the end, instead of once per pointer.
    FreeNode* head = freeList_;
    std::size_t released = 0;

    for (void* ptr : ptrs) {
        if (!ptr || !owns(ptr)) [[unlikely]] {
            continue;
        }

        auto* node = static_cast<FreeNode*>(ptr);
        node->next = head;
        head = node;
        ++released;
    }

    freeList_ = head;
    freeBlockCount_ += released;
    if (released)
        statDealloc(released);
}

// ============================================================
//  Section 4 — Object Lifecycle
// ============================================================

template <bool EnableStats>
template <typename T, typename... Args>
    requires std::constructible_from<T, Args...>
T* Pool<EnableStats>::create(Args&&... args) {
    AP_PRE(sizeof(T) <= blockSize_);
    AP_PRE(alignof(T) <= alignment_);

    void* block = allocate();
    if (!block)
        return nullptr;

    if constexpr (std::is_nothrow_constructible_v<T, Args...>) {
        return ::new (block) T(std::forward<Args>(args)...);
    } else {
        // T's constructor can throw: if it does, hand the block back
        // before the exception propagates so it is never lost.
        try {
            return ::new (block) T(std::forward<Args>(args)...);
        } catch (...) {
            deallocate(block);
            throw;
        }
    }
}

template <bool EnableStats> template <typename T> void Pool<EnableStats>::destroy(T* ptr) noexcept {
    if (ptr == nullptr || !owns(ptr))
        return;

    ptr->~T();
    deallocate(ptr);
}

// ============================================================
//  Section 5 — Pool Management
// ============================================================

template <bool EnableStats> void Pool<EnableStats>::reset() noexcept {
    initializeFreeList();
    if constexpr (EnableStats)
        stats_ = Stats{};
}

// ============================================================
//  Section 6 — Introspection
// ============================================================

template <bool EnableStats> bool Pool<EnableStats>::owns(const void* ptr) const noexcept {
    if (memory_ == nullptr) [[unlikely]] {
        // Moved-from / zero-capacity pool: memory_ and memoryEnd_ are both
        // null. Bail out before the pointer comparisons below, since
        // comparing an arbitrary caller pointer against nullptr with
        // </>= is only well-defined when both pointers are within the
        // same array — which nullptr never is.
        return false;
    }

    const auto* p = static_cast<const std::byte*>(ptr);
    if (p < memory_ || p >= memoryEnd_)
        return false;

    const std::size_t diff = static_cast<std::size_t>(p - memory_);
    return strideIsPow2_ ? (diff & strideMask_) == 0 : (diff % stride_) == 0;
}

template <bool EnableStats>
const typename Pool<EnableStats>::Stats& Pool<EnableStats>::getStats() const noexcept
    requires EnableStats
{
    return stats_;
}

template <bool EnableStats> std::size_t Pool<EnableStats>::capacity() const noexcept {
    return static_cast<std::size_t>(memoryEnd_ - memory_);
}

template <bool EnableStats> std::size_t Pool<EnableStats>::usedBlocks() const noexcept {
    return blockCount_ - freeBlockCount_;
}

template <bool EnableStats> std::size_t Pool<EnableStats>::freeBlocks() const noexcept {
    return freeBlockCount_;
}

template <bool EnableStats> std::size_t Pool<EnableStats>::totalBlocks() const noexcept {
    return blockCount_;
}

template <bool EnableStats> std::size_t Pool<EnableStats>::blockStride() const noexcept {
    return stride_;
}

// ============================================================
//  Section 7 — Utility Functions
// ============================================================

template <bool EnableStats>
constexpr bool Pool<EnableStats>::isPowerOfTwo(std::size_t value) noexcept {
    return value != 0 && (value & (value - 1)) == 0;
}

template <bool EnableStats>
constexpr std::size_t Pool<EnableStats>::alignForward(std::size_t value,
                                                      std::size_t alignment) noexcept {
    return (value + alignment - 1) & ~(alignment - 1);
}

template <bool EnableStats>
constexpr std::size_t Pool<EnableStats>::validateAndComputeStride(std::size_t blockSize,
                                                                  std::size_t blockCount,
                                                                  std::size_t alignment) noexcept {
    AP_PRE(blockCount > 0);
    AP_PRE(isPowerOfTwo(alignment));
    AP_PRE(blockSize >= sizeof(FreeNode));
    return alignForward(blockSize, alignment);
}

template <bool EnableStats>
std::byte* Pool<EnableStats>::allocateStorage(std::size_t stride, std::size_t blockCount,
                                              std::size_t alignment) {
    AP_PRE(stride == 0 || blockCount <= std::numeric_limits<std::size_t>::max() / stride);
    return static_cast<std::byte*>(
        ::operator new(stride * blockCount, std::align_val_t(alignment)));
}

// ============================================================
//  Section 8 — Free List Helpers
// ============================================================

template <bool EnableStats> void* Pool<EnableStats>::acquireBlock() noexcept {
    if (freeList_) {
        FreeNode* node = freeList_;
        freeList_ = node->next;
        AP_PREFETCH(freeList_);
        --freeBlockCount_;
        return node;
    }

    if (watermark_ < blockCount_) {
        std::byte* block = memory_ + watermark_ * stride_;
        ++watermark_;
        --freeBlockCount_;
        return block;
    }

    return nullptr;
}

template <bool EnableStats> void Pool<EnableStats>::releaseBlock(void* ptr) noexcept {
    AP_ASSERT(ptr != freeList_); // catches an immediate double-free at zero release cost
    auto* node = static_cast<FreeNode*>(ptr);
    node->next = freeList_;
    freeList_ = node;
    ++freeBlockCount_;
}

template <bool EnableStats> void Pool<EnableStats>::initializeFreeList() noexcept {
    freeList_ = nullptr;
    watermark_ = 0;
    freeBlockCount_ = blockCount_;
}

// ============================================================
//  Section 9 — Statistics Helpers
// ============================================================

template <bool EnableStats>
constexpr void Pool<EnableStats>::statAlloc(std::size_t usedNow, std::size_t count) noexcept {
    if constexpr (EnableStats) {
        stats_.totalAllocated_ += count;
        stats_.allocations_ += count;
        if (usedNow > stats_.peakUsed_)
            stats_.peakUsed_ = usedNow;
    }
}

template <bool EnableStats>
constexpr void Pool<EnableStats>::statDealloc(std::size_t count) noexcept {
    if constexpr (EnableStats)
        stats_.deallocations_ += count;
}

} // namespace PoolPro
