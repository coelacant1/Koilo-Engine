// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file asset_handle.hpp
 * @brief Lightweight generational handle for referencing loaded assets.
 *
 * AssetHandle is a 32-bit value type that encodes a slot index, generation
 * counter, and asset type. The generation counter detects stale references
 * after an asset slot is recycled.
 *
 * @date 10/28/2025
 * @author Coela
 */
#pragma once

#include <cstdint>
#include <functional>

namespace koilo {

/// Broad asset categories for fast filtering.
enum class AssetType : uint8_t {
    Mesh     = 0,
    Texture  = 1,
    Audio    = 2,
    Script   = 3,
    Font     = 4,
    Markup   = 5,
    Material = 6,
    Generic  = 7
};

/// Lifecycle state of an asset slot.
enum class AssetState : uint8_t {
    Empty,      ///< Slot is free / never used
    Registered, ///< Path registered but data not loaded
    Loading,    ///< Async load in progress
    Loaded,     ///< Data available
    Failed,     ///< Load attempted and failed
    Unloading   ///< Being torn down
};

/**
 * @struct AssetHandle
 * @brief Generational index into the asset registry.
 *
 * Layout (32 bits):
 *   [31..12] index      - 20 bits -> 1,048,576 max asset slots
 *   [11..3]  generation - 9 bits  -> 512 reuses before wrap
 *   [2..0]   type       - 3 bits  -> 8 AssetType values
 */
struct AssetHandle {
    uint32_t value = 0;

    AssetHandle() = default;

    static AssetHandle Make(uint32_t index, uint32_t generation, AssetType type) {
        AssetHandle h;
        h.value = ((index & 0xFFFFF) << 12)
                | ((generation & 0x1FF) << 3)
                | (static_cast<uint32_t>(type) & 0x7);
        return h;
    }

    uint32_t  Index()      const { return (value >> 12) & 0xFFFFF; }
    uint32_t  Generation() const { return (value >> 3) & 0x1FF; }
    AssetType Type()       const { return static_cast<AssetType>(value & 0x7); }

    bool IsNull() const { return value == 0; }

    bool operator==(AssetHandle other) const { return value == other.value; }
    bool operator!=(AssetHandle other) const { return value != other.value; }
    bool operator<(AssetHandle other)  const { return value < other.value; }

    explicit operator bool() const { return !IsNull(); }

    /// Sentinel value representing no asset.
    static AssetHandle Null() { return AssetHandle{}; }
};

/// Return a human-readable name for an AssetType.
inline const char* AssetTypeName(AssetType t) {
    switch (t) {
        case AssetType::Mesh:     return "Mesh";
        case AssetType::Texture:  return "Texture";
        case AssetType::Audio:    return "Audio";
        case AssetType::Script:   return "Script";
        case AssetType::Font:     return "Font";
        case AssetType::Markup:   return "Markup";
        case AssetType::Material: return "Material";
        case AssetType::Generic:  return "Generic";
    }
    return "Unknown";
}

/// Return a human-readable name for an AssetState.
inline const char* AssetStateName(AssetState s) {
    switch (s) {
        case AssetState::Empty:      return "Empty";
        case AssetState::Registered: return "Registered";
        case AssetState::Loading:    return "Loading";
        case AssetState::Loaded:     return "Loaded";
        case AssetState::Failed:     return "Failed";
        case AssetState::Unloading:  return "Unloading";
    }
    return "Unknown";
}

} // namespace koilo

// Hash support for use in unordered containers.
namespace std {
template<>
struct hash<koilo::AssetHandle> {
    size_t operator()(koilo::AssetHandle h) const noexcept {
        return hash<uint32_t>{}(h.value);
    }
};
} // namespace std
