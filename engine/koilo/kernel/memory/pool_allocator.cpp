#include <koilo/kernel/memory/pool_allocator.hpp>
#include <cstdlib>
#include <cassert>

namespace koilo {

PoolAllocator::PoolAllocator(size_t blockSize, size_t blockCount)
    : blockSize_(blockSize < sizeof(void*) ? sizeof(void*) : blockSize)
    , blockCount_(blockCount)
    , freeList_(nullptr)
    , usedCount_(0)
    , ownsMemory_(true) {
    buffer_ = static_cast<uint8_t*>(std::malloc(blockSize_ * blockCount_));
    BuildFreeList();
}

PoolAllocator::PoolAllocator(void* backing, size_t blockSize, size_t blockCount)
    : buffer_(static_cast<uint8_t*>(backing))
    , blockSize_(blockSize < sizeof(void*) ? sizeof(void*) : blockSize)
    , blockCount_(blockCount)
    , freeList_(nullptr)
    , usedCount_(0)
    , ownsMemory_(false) {
    BuildFreeList();
}

PoolAllocator::~PoolAllocator() {
    if (ownsMemory_ && buffer_) {
        std::free(buffer_);
    }
}

void PoolAllocator::BuildFreeList() {
    freeList_ = nullptr;
    // Link blocks in reverse order so first allocation returns first block
    for (size_t i = blockCount_; i > 0; --i) {
        void* block = buffer_ + (i - 1) * blockSize_;
        *static_cast<void**>(block) = freeList_;
        freeList_ = block;
    }
    usedCount_ = 0;
}

void* PoolAllocator::Allocate() {
    if (!freeList_) return nullptr;

    void* block = freeList_;
    freeList_ = *static_cast<void**>(block);
    ++usedCount_;
    return block;
}

void PoolAllocator::Free(void* ptr) {
    if (!ptr) return;
    assert(Owns(ptr) && "PoolAllocator::Free: pointer not from this pool");

    *static_cast<void**>(ptr) = freeList_;
    freeList_ = ptr;
    --usedCount_;
}

void PoolAllocator::Reset() {
    BuildFreeList();
}

bool PoolAllocator::Owns(const void* ptr) const {
    auto p = static_cast<const uint8_t*>(ptr);
    return p >= buffer_ && p < buffer_ + blockSize_ * blockCount_;
}

} // namespace koilo
