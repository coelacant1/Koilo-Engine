// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file arena_allocator.hpp
 * @brief Bump allocator with bulk-free for per-frame scratch memory.
 *
 * @date 10/19/2025
 * @author Coela
 */
#pragma once
#include <cstddef>
#include <cstdint>

namespace koilo {

/// Bump allocator with bulk-free. Ideal for per-frame scratch memory.
/// Allocations are contiguous; Reset() frees everything at once (O(1)).
class ArenaAllocator {
public:
    /// Allocate a new backing buffer of the given capacity.
    explicit ArenaAllocator(size_t capacity);

    /// Use an externally-owned backing buffer (bare metal / static memory).
    ArenaAllocator(void* backing, size_t capacity);

    ~ArenaAllocator();

    ArenaAllocator(const ArenaAllocator&) = delete;
    ArenaAllocator& operator=(const ArenaAllocator&) = delete;

    /// Allocate `size` bytes with the given alignment. Returns nullptr on overflow.
    void* Allocate(size_t size, size_t alignment = 8);

    /// Typed convenience: allocate sizeof(T) * count, properly aligned.
    template<typename T>
    T* Allocate(size_t count = 1) {
        return static_cast<T*>(Allocate(sizeof(T) * count, alignof(T)));
    }

    /// Free all allocations at once (O(1)).
    void Reset();

    size_t Used() const     { return offset_; }
    size_t Capacity() const { return capacity_; }
    size_t Remaining() const{ return capacity_ - offset_; }

private:
    uint8_t* buffer_;
    size_t   capacity_;
    size_t   offset_;
    bool     ownsMemory_;
};

} // namespace koilo
