// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file linear_allocator.hpp
 * @brief Bump allocator with save/restore markers for nested scopes.
 *
 * @date 10/14/2025
 * @author Coela
 */
#pragma once
#include <cstddef>
#include <cstdint>

namespace koilo {

/// Bump allocator with save/restore markers for nested scopes.
/// Like ArenaAllocator but supports partial rollback via Markers.
class LinearAllocator {
public:
    /// Allocate a new backing buffer of the given capacity.
    explicit LinearAllocator(size_t capacity);

    /// Use an externally-owned backing buffer (bare metal / static memory).
    LinearAllocator(void* backing, size_t capacity);

    ~LinearAllocator();

    LinearAllocator(const LinearAllocator&) = delete;
    LinearAllocator& operator=(const LinearAllocator&) = delete;

    /// Allocate `size` bytes with the given alignment. Returns nullptr on overflow.
    void* Allocate(size_t size, size_t alignment = 8);

    /// Typed convenience.
    template<typename T>
    T* Allocate(size_t count = 1) {
        return static_cast<T*>(Allocate(sizeof(T) * count, alignof(T)));
    }

    /// Opaque save point for partial rollback.
    struct Marker {
        size_t offset;
    };

    /// Save the current allocation position.
    Marker GetMarker() const { return {offset_}; }

    /// Roll back all allocations made after this marker.
    void ResetToMarker(Marker marker);

    /// Reset everything (equivalent to ResetToMarker({0})).
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
