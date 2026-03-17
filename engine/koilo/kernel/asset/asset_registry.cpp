// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file asset_registry.cpp
 * @brief AssetRegistry implementation - slot management, lookup, dependencies.
 */
#include <koilo/kernel/asset/asset_registry.hpp>
#include <algorithm>

namespace koilo {

const std::string AssetRegistry::kEmptyString_;

AssetRegistry::AssetRegistry() {
    slots_.reserve(256);
}

AssetRegistry::~AssetRegistry() = default;

// -- Slot allocation -----------------------------------------------

uint32_t AssetRegistry::AllocateSlot() {
    if (!freeList_.empty()) {
        uint32_t idx = freeList_.back();
        freeList_.pop_back();
        return idx;
    }
    uint32_t idx = static_cast<uint32_t>(slots_.size());
    slots_.emplace_back();
    return idx;
}

void AssetRegistry::ReleaseSlot(uint32_t index) {
    freeList_.push_back(index);
}

AssetSlot* AssetRegistry::GetSlot(AssetHandle handle) {
    if (handle.IsNull()) return nullptr;
    uint32_t idx = handle.Index();
    if (idx >= slots_.size()) return nullptr;
    AssetSlot& slot = slots_[idx];
    if (slot.state == AssetState::Empty) return nullptr;
    if (slot.generation != handle.Generation()) return nullptr;
    return &slot;
}

const AssetSlot* AssetRegistry::GetSlot(AssetHandle handle) const {
    if (handle.IsNull()) return nullptr;
    uint32_t idx = handle.Index();
    if (idx >= slots_.size()) return nullptr;
    const AssetSlot& slot = slots_[idx];
    if (slot.state == AssetState::Empty) return nullptr;
    if (slot.generation != handle.Generation()) return nullptr;
    return &slot;
}

// -- Registration --------------------------------------------------

AssetHandle AssetRegistry::Register(const std::string& path, AssetType type) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Dedup: if already registered, return existing handle.
    auto it = pathIndex_.find(path);
    if (it != pathIndex_.end()) {
        return it->second;
    }

    uint32_t idx = AllocateSlot();
    AssetSlot& slot = slots_[idx];
    // generation was already bumped on release (or is 0 for fresh slots)
    slot.path        = path;
    slot.type        = type;
    slot.state       = AssetState::Registered;
    slot.data        = nullptr;
    slot.memoryBytes = 0;
    slot.lastModified = 0;
    slot.dependencies.clear();

    AssetHandle handle = AssetHandle::Make(idx, slot.generation, type);
    pathIndex_[path] = handle;
    return handle;
}

void AssetRegistry::Unregister(AssetHandle handle) {
    std::lock_guard<std::mutex> lock(mutex_);

    AssetSlot* slot = GetSlot(handle);
    if (!slot) return;

    uint32_t idx = handle.Index();

    // Remove from path index.
    pathIndex_.erase(slot->path);

    // Remove from reverse-dep map (as a dependency of others).
    reverseDeps_.erase(idx);

    // Remove this asset from any other asset's dependency list.
    for (AssetHandle dep : slot->dependencies) {
        auto rit = reverseDeps_.find(dep.Index());
        if (rit != reverseDeps_.end()) {
            auto& vec = rit->second;
            vec.erase(std::remove(vec.begin(), vec.end(), handle), vec.end());
        }
    }

    // Clear slot and bump generation for stale-handle detection.
    slot->path.clear();
    slot->type  = AssetType::Generic;
    slot->state = AssetState::Empty;
    slot->data.reset();
    slot->memoryBytes = 0;
    slot->lastModified = 0;
    slot->dependencies.clear();
    slot->generation = (slot->generation + 1) & 0x1FF; // 9-bit wrap

    ReleaseSlot(idx);
}

// -- Validity ------------------------------------------------------

bool AssetRegistry::IsValid(AssetHandle handle) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return GetSlot(handle) != nullptr;
}

// -- State management ----------------------------------------------

void AssetRegistry::SetState(AssetHandle handle, AssetState state) {
    std::lock_guard<std::mutex> lock(mutex_);
    AssetSlot* slot = GetSlot(handle);
    if (slot) slot->state = state;
}

AssetState AssetRegistry::GetState(AssetHandle handle) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const AssetSlot* slot = GetSlot(handle);
    return slot ? slot->state : AssetState::Empty;
}

void AssetRegistry::SetLastModified(AssetHandle handle, time_t t) {
    std::lock_guard<std::mutex> lock(mutex_);
    AssetSlot* slot = GetSlot(handle);
    if (slot) slot->lastModified = t;
}

time_t AssetRegistry::GetLastModified(AssetHandle handle) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const AssetSlot* slot = GetSlot(handle);
    return slot ? slot->lastModified : 0;
}

// -- Lookup --------------------------------------------------------

AssetHandle AssetRegistry::FindByPath(const std::string& path) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = pathIndex_.find(path);
    return (it != pathIndex_.end()) ? it->second : AssetHandle::Null();
}

std::vector<AssetHandle> AssetRegistry::FindByType(AssetType type) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<AssetHandle> result;
    for (uint32_t i = 0; i < slots_.size(); ++i) {
        const AssetSlot& slot = slots_[i];
        if (slot.state != AssetState::Empty && slot.type == type) {
            result.push_back(AssetHandle::Make(i, slot.generation, slot.type));
        }
    }
    return result;
}

std::vector<AssetHandle> AssetRegistry::FindByState(AssetState state) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<AssetHandle> result;
    for (uint32_t i = 0; i < slots_.size(); ++i) {
        const AssetSlot& slot = slots_[i];
        if (slot.state == state) {
            result.push_back(AssetHandle::Make(i, slot.generation, slot.type));
        }
    }
    return result;
}

const std::string& AssetRegistry::GetPath(AssetHandle handle) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const AssetSlot* slot = GetSlot(handle);
    return slot ? slot->path : kEmptyString_;
}

size_t AssetRegistry::GetMemoryBytes(AssetHandle handle) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const AssetSlot* slot = GetSlot(handle);
    return slot ? slot->memoryBytes : 0;
}

std::shared_ptr<void> AssetRegistry::GetRawData(AssetHandle handle) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const AssetSlot* slot = GetSlot(handle);
    return slot ? slot->data : nullptr;
}

// -- Dependency graph ----------------------------------------------

void AssetRegistry::AddDependency(AssetHandle asset, AssetHandle dependency) {
    std::lock_guard<std::mutex> lock(mutex_);
    AssetSlot* slot = GetSlot(asset);
    if (!slot) return;
    if (!GetSlot(dependency)) return;

    // Avoid duplicates.
    auto& deps = slot->dependencies;
    if (std::find(deps.begin(), deps.end(), dependency) != deps.end()) return;

    deps.push_back(dependency);
    reverseDeps_[dependency.Index()].push_back(asset);
}

void AssetRegistry::RemoveDependency(AssetHandle asset, AssetHandle dependency) {
    std::lock_guard<std::mutex> lock(mutex_);
    AssetSlot* slot = GetSlot(asset);
    if (!slot) return;

    auto& deps = slot->dependencies;
    deps.erase(std::remove(deps.begin(), deps.end(), dependency), deps.end());

    auto rit = reverseDeps_.find(dependency.Index());
    if (rit != reverseDeps_.end()) {
        auto& vec = rit->second;
        vec.erase(std::remove(vec.begin(), vec.end(), asset), vec.end());
    }
}

std::vector<AssetHandle> AssetRegistry::GetDependencies(AssetHandle asset) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const AssetSlot* slot = GetSlot(asset);
    return slot ? slot->dependencies : std::vector<AssetHandle>{};
}

std::vector<AssetHandle> AssetRegistry::GetDependents(AssetHandle asset) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = reverseDeps_.find(asset.Index());
    if (it == reverseDeps_.end()) return {};
    return it->second;
}

// -- Bulk queries --------------------------------------------------

size_t AssetRegistry::TotalMemory() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t total = 0;
    for (const auto& slot : slots_) {
        if (slot.state != AssetState::Empty) {
            total += slot.memoryBytes;
        }
    }
    return total;
}

size_t AssetRegistry::Count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& slot : slots_) {
        if (slot.state != AssetState::Empty) ++count;
    }
    return count;
}

size_t AssetRegistry::CountByState(AssetState state) const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& slot : slots_) {
        if (slot.state == state) ++count;
    }
    return count;
}

void AssetRegistry::ForEach(SlotVisitor visitor) const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (uint32_t i = 0; i < slots_.size(); ++i) {
        const AssetSlot& slot = slots_[i];
        if (slot.state != AssetState::Empty) {
            visitor(AssetHandle::Make(i, slot.generation, slot.type), slot);
        }
    }
}

std::vector<AssetHandle> AssetRegistry::CollectGarbage() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<AssetHandle> candidates;
    for (uint32_t i = 0; i < slots_.size(); ++i) {
        const AssetSlot& slot = slots_[i];
        if (slot.state == AssetState::Loaded && slot.data && slot.data.use_count() == 1) {
            candidates.push_back(AssetHandle::Make(i, slot.generation, slot.type));
        }
    }
    return candidates;
}

} // namespace koilo
