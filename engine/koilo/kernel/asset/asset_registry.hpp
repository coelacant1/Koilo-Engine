// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file asset_registry.hpp
 * @brief Central registry mapping AssetHandles to loaded asset data.
 *
 * The registry owns a dense array of AssetSlots. Each slot holds metadata
 * (path, type, state, memory size) and a type-erased shared_ptr to the
 * loaded data. Generational indices detect stale handles.
 *
 * Dependency tracking allows cascading hot-reload invalidation:
 * if texture T is a dependency of material M, reloading T can trigger
 * a reload of M.
 *
 * @date 11/15/2025
 * @author Coela
 */
#pragma once

#include <koilo/kernel/asset/asset_handle.hpp>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <ctime>
#include <mutex>
#include <functional>

namespace koilo {

/**
 * @struct AssetSlot
 * @brief Internal storage for one registered asset.
 */
struct AssetSlot {
    std::string         path;           ///< Relative path from project root
    AssetType           type    = AssetType::Generic;
    AssetState          state   = AssetState::Empty;
    uint32_t            generation = 0; ///< Incremented each time slot is recycled
    std::shared_ptr<void> data;         ///< Type-erased loaded asset data
    size_t              memoryBytes = 0;
    time_t              lastModified = 0;
    std::vector<AssetHandle> dependencies; ///< Assets this asset depends on
};

/**
 * @class AssetRegistry
 * @brief Maps AssetHandles to AssetSlots; provides registration, lookup,
 *        and dependency tracking.
 *
 * Thread-safe: all public methods acquire an internal mutex.
 */
class AssetRegistry {
public:
    AssetRegistry();
    ~AssetRegistry();

    // -- Registration ----------------------------------------------

    /// Register a new asset path. Returns a handle for future reference.
    /// If the path is already registered, returns the existing handle.
    AssetHandle Register(const std::string& path, AssetType type);

    /// Unregister an asset, freeing its slot for reuse.
    /// Invalidates the handle (generation bumps).
    void Unregister(AssetHandle handle);

    // -- Validity --------------------------------------------------

    /// Check whether a handle refers to a live slot with matching generation.
    bool IsValid(AssetHandle handle) const;

    // -- Data access -----------------------------------------------

    /// Store loaded data for an asset.
    template<typename T>
    void SetData(AssetHandle handle, std::shared_ptr<T> data, size_t memoryBytes = 0);

    /// Retrieve loaded data (returns nullptr if handle is invalid or no data).
    template<typename T>
    T* Get(AssetHandle handle) const;

    /// Retrieve the shared_ptr (type-erased).
    std::shared_ptr<void> GetRawData(AssetHandle handle) const;

    // -- State management ------------------------------------------

    /// Set the state of an asset slot.
    void SetState(AssetHandle handle, AssetState state);

    /// Get the state of an asset slot.
    AssetState GetState(AssetHandle handle) const;

    /// Set the last-modified timestamp.
    void SetLastModified(AssetHandle handle, time_t t);

    /// Get the last-modified timestamp.
    time_t GetLastModified(AssetHandle handle) const;

    // -- Lookup ----------------------------------------------------

    /// Find a handle by path (returns Null handle if not found).
    AssetHandle FindByPath(const std::string& path) const;

    /// Find all handles matching a given type.
    std::vector<AssetHandle> FindByType(AssetType type) const;

    /// Find all handles in a given state.
    std::vector<AssetHandle> FindByState(AssetState state) const;

    /// Get the path for a handle (empty string if invalid).
    const std::string& GetPath(AssetHandle handle) const;

    /// Get memory usage for a single asset.
    size_t GetMemoryBytes(AssetHandle handle) const;

    // -- Dependency graph ------------------------------------------

    /// Declare that `asset` depends on `dependency`.
    void AddDependency(AssetHandle asset, AssetHandle dependency);

    /// Remove a dependency link.
    void RemoveDependency(AssetHandle asset, AssetHandle dependency);

    /// Get direct dependencies of an asset.
    std::vector<AssetHandle> GetDependencies(AssetHandle asset) const;

    /// Get assets that depend on the given asset (reverse lookup).
    std::vector<AssetHandle> GetDependents(AssetHandle asset) const;

    // -- Bulk queries ----------------------------------------------

    /// Total memory across all loaded assets.
    size_t TotalMemory() const;

    /// Number of registered (non-empty) slots.
    size_t Count() const;

    /// Number of slots in a given state.
    size_t CountByState(AssetState state) const;

    /// Iterate all registered assets. Callback receives handle + const slot ref.
    using SlotVisitor = std::function<void(AssetHandle, const AssetSlot&)>;
    void ForEach(SlotVisitor visitor) const;

    /// Collect handles whose slot has use_count == 1 (only registry holds data).
    std::vector<AssetHandle> CollectGarbage() const;

private:
    mutable std::mutex mutex_;

    std::vector<AssetSlot> slots_;
    std::vector<uint32_t>  freeList_;  ///< Recycled slot indices

    /// Path -> handle for O(1) dedup lookup.
    std::unordered_map<std::string, AssetHandle> pathIndex_;

    /// Reverse dependency map: dependency -> list of assets that depend on it.
    std::unordered_map<uint32_t, std::vector<AssetHandle>> reverseDeps_;

    static const std::string kEmptyString_;

    /// Allocate or recycle a slot index.
    uint32_t AllocateSlot();

    /// Return a slot index to the free list.
    void ReleaseSlot(uint32_t index);

    /// Get a mutable slot pointer (no lock - caller must hold mutex_).
    AssetSlot* GetSlot(AssetHandle handle);

    /// Get a const slot pointer (no lock - caller must hold mutex_).
    const AssetSlot* GetSlot(AssetHandle handle) const;
};

// -- Template implementations --------------------------------------

template<typename T>
void AssetRegistry::SetData(AssetHandle handle, std::shared_ptr<T> data, size_t memoryBytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    AssetSlot* slot = GetSlot(handle);
    if (!slot) return;
    slot->data = std::static_pointer_cast<void>(data);
    slot->memoryBytes = memoryBytes;
    slot->state = AssetState::Loaded;
}

template<typename T>
T* AssetRegistry::Get(AssetHandle handle) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const AssetSlot* slot = GetSlot(handle);
    if (!slot || !slot->data) return nullptr;
    return static_cast<T*>(slot->data.get());
}

} // namespace koilo
