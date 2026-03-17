// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file asset_manifest.hpp
 * @brief Catalog of known assets with metadata, persisted as .kasset files.
 *
 * The manifest is a flat list of AssetEntry records that can be scanned
 * from a project directory, saved to disk, and reloaded. It provides
 * fast lookup by path and filtering by type.
 *
 * @date 12/04/2025
 * @author Coela
 */
#pragma once

#include <koilo/kernel/asset/asset_handle.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include "../../registry/reflect_macros.hpp"

namespace koilo {

/**
 * @struct AssetEntry
 * @brief Metadata for one asset in the manifest.
 */
struct AssetEntry {
    std::string path;            ///< Relative path from project root
    AssetType   type = AssetType::Generic;
    size_t      sizeBytes = 0;   ///< File size on disk
    uint64_t    contentHash = 0; ///< FNV-1a hash of file contents
    std::string converterName;   ///< Converter that produced this (empty if native)
    std::string sourcePath;      ///< Original source path (empty if native)

    KL_BEGIN_FIELDS(AssetEntry)
        KL_FIELD(AssetEntry, path, "Path", 0, 0),
        KL_FIELD(AssetEntry, sizeBytes, "Size bytes", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(AssetEntry)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(AssetEntry)
        /* No reflected ctors. */
    KL_END_DESCRIBE(AssetEntry)

};

/**
 * @class AssetManifest
 * @brief Catalog of project assets. Supports scan, save, load, and query.
 */
class AssetManifest {
public:
    AssetManifest() = default;
    ~AssetManifest() = default;

    // -- Persistence -----------------------------------------------

    /// Load manifest from a binary .kasset file. Returns true on success.
    bool Load(const std::string& manifestPath);

    /// Save manifest to a binary .kasset file. Returns true on success.
    bool Save(const std::string& manifestPath) const;

    // -- Discovery -------------------------------------------------

    /// Recursively scan a directory for assets, adding entries for known
    /// file extensions. Clears existing entries first.
    void Scan(const std::string& directory);

    /// Add or update a single entry. If path already exists, updates it.
    void AddOrUpdate(const AssetEntry& entry);

    /// Remove an entry by path.
    bool Remove(const std::string& path);

    // -- Query -----------------------------------------------------

    /// Find an entry by path (returns nullptr if not found).
    const AssetEntry* Find(const std::string& path) const;

    /// Find all entries matching a given type.
    std::vector<const AssetEntry*> FindByType(AssetType type) const;

    /// Total number of entries.
    size_t Count() const { return entries_.size(); }

    /// Total bytes across all entries.
    size_t TotalBytes() const;

    /// Access all entries (read-only).
    const std::vector<AssetEntry>& Entries() const { return entries_; }

    /// Clear all entries.
    void Clear();

    // -- Utility ---------------------------------------------------

    /// Infer AssetType from a file extension.
    static AssetType InferType(const std::string& extension);

    /// Compute FNV-1a hash of file contents.
    static uint64_t HashFile(const std::string& path);

private:
    std::vector<AssetEntry> entries_;

    /// Path -> index for O(1) lookup.
    std::unordered_map<std::string, size_t> pathIndex_;

    void RebuildIndex();

    KL_BEGIN_FIELDS(AssetManifest)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(AssetManifest)
        KL_METHOD_AUTO(AssetManifest, TotalBytes, "Total bytes"),
        KL_METHOD_AUTO(AssetManifest, Clear, "Clear")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(AssetManifest)
        KL_CTOR0(AssetManifest)
    KL_END_DESCRIBE(AssetManifest)

};

} // namespace koilo
