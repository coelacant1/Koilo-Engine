#include <koilo/kernel/memory/arena_allocator.hpp>
#include <cstdlib>
#include <cstring>

namespace koilo {

ArenaAllocator::ArenaAllocator(size_t capacity)
    : buffer_(static_cast<uint8_t*>(std::malloc(capacity)))
    , capacity_(capacity)
    , offset_(0)
    , ownsMemory_(true) {
}

ArenaAllocator::ArenaAllocator(void* backing, size_t capacity)
    : buffer_(static_cast<uint8_t*>(backing))
    , capacity_(capacity)
    , offset_(0)
    , ownsMemory_(false) {
}

ArenaAllocator::~ArenaAllocator() {
    if (ownsMemory_ && buffer_) {
        std::free(buffer_);
    }
}

void* ArenaAllocator::Allocate(size_t size, size_t alignment) {
    // Align the current offset upward
    size_t aligned = (offset_ + alignment - 1) & ~(alignment - 1);
    if (aligned + size > capacity_) {
        return nullptr;
    }
    void* ptr = buffer_ + aligned;
    offset_ = aligned + size;
    return ptr;
}

void ArenaAllocator::Reset() {
    offset_ = 0;
}

} // namespace koilo
