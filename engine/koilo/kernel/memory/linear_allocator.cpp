#include <koilo/kernel/memory/linear_allocator.hpp>
#include <cstdlib>
#include <cassert>

namespace koilo {

LinearAllocator::LinearAllocator(size_t capacity)
    : buffer_(static_cast<uint8_t*>(std::malloc(capacity)))
    , capacity_(capacity)
    , offset_(0)
    , ownsMemory_(true) {
}

LinearAllocator::LinearAllocator(void* backing, size_t capacity)
    : buffer_(static_cast<uint8_t*>(backing))
    , capacity_(capacity)
    , offset_(0)
    , ownsMemory_(false) {
}

LinearAllocator::~LinearAllocator() {
    if (ownsMemory_ && buffer_) {
        std::free(buffer_);
    }
}

void* LinearAllocator::Allocate(size_t size, size_t alignment) {
    size_t aligned = (offset_ + alignment - 1) & ~(alignment - 1);
    if (aligned + size > capacity_) {
        return nullptr;
    }
    void* ptr = buffer_ + aligned;
    offset_ = aligned + size;
    return ptr;
}

void LinearAllocator::ResetToMarker(Marker marker) {
    assert(marker.offset <= offset_ && "LinearAllocator: marker is ahead of current offset");
    offset_ = marker.offset;
}

void LinearAllocator::Reset() {
    offset_ = 0;
}

} // namespace koilo
