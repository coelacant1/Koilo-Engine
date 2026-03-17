// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file pool_allocator.hpp
 * @brief Fixed-size block allocator using an embedded free list.
 *
 * @date 10/22/2025
 * @author Coela
 */
#pragma once
#include <cstddef>
#include <cstdint>

namespace koilo {

/// Fixed-size block allocator using an embedded free list.
/// O(1) allocate and free. Ideal for entities, components, widgets.
class PoolAllocator {
public:
    /// Allocate backing buffer for `blockCount` blocks of `blockSize` bytes.
    PoolAllocator(size_t blockSize, size_t blockCount);

    /// Use an externally-owned backing buffer (bare metal / static memory).
    /// Buffer must be at least blockSize * blockCount bytes.
    PoolAllocator(void* backing, size_t blockSize, size_t blockCount);

    ~PoolAllocator();

    PoolAllocator(const PoolAllocator&) = delete;
    PoolAllocator& operator=(const PoolAllocator&) = delete;

    /// Allocate one block. Returns nullptr if pool is exhausted.
    void* Allocate();

    /// Return a block to the pool. Must have been allocated from this pool.
    void Free(void* ptr);

    /// Typed convenience.
    template<typename T>
    T* Allocate() {
        return static_cast<T*>(Allocate());
    }

    /// Reset pool: all blocks returned to free list. Invalidates all pointers.
    void Reset();

    size_t BlockSize() const   { return blockSize_; }
    size_t TotalBlocks() const { return blockCount_; }
    size_t UsedBlocks() const  { return usedCount_; }
    size_t FreeBlocks() const  { return blockCount_ - usedCount_; }

    /// Check if a pointer belongs to this pool.
    bool Owns(const void* ptr) const;

private:
    void BuildFreeList();

    uint8_t* buffer_;
    size_t   blockSize_;
    size_t   blockCount_;
    void*    freeList_;
    size_t   usedCount_;
    bool     ownsMemory_;
};

} // namespace koilo
