// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file asset_manifest.cpp
 * @brief AssetManifest implementation - scan, persist, query.
 */
#include <koilo/kernel/asset/asset_manifest.hpp>
#include <koilo/kernel/schema_version.hpp>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cstring>

namespace fs = std::filesystem;

namespace koilo {

// -- Binary format -------------------------------------------------
// Header:  "KAST" (4 bytes) + version (uint32) + entry count (uint32)
// Entries: [pathLen(uint16) path(N) type(uint8) sizeBytes(uint64)
//           contentHash(uint64) converterLen(uint16) converter(N)
//           sourceLen(uint16) source(N)]

static constexpr char kManifestMagic[4] = {'K','A','S','T'};
static constexpr uint32_t kManifestVersion = 1;

static void WriteString(std::ofstream& f, const std::string& s) {
    uint16_t len = static_cast<uint16_t>(s.size());
    f.write(reinterpret_cast<const char*>(&len), 2);
    if (len > 0) f.write(s.data(), len);
}

static std::string ReadString(std::ifstream& f) {
    uint16_t len = 0;
    f.read(reinterpret_cast<char*>(&len), 2);
    if (len == 0) return {};
    std::string s(len, '\0');
    f.read(s.data(), len);
    return s;
}

bool AssetManifest::Load(const std::string& manifestPath) {
    std::ifstream f(manifestPath, std::ios::binary);
    if (!f.is_open()) return false;

    auto hdr = ReadSchemaHeader(f, kManifestMagic);
    if (!hdr.valid) return false;
    if (hdr.version != kManifestVersion) return false;

    uint32_t count = 0;
    f.read(reinterpret_cast<char*>(&count), 4);

    entries_.clear();
    entries_.reserve(count);

    for (uint32_t i = 0; i < count; ++i) {
        AssetEntry entry;
        entry.path = ReadString(f);
        uint8_t type = 0;
        f.read(reinterpret_cast<char*>(&type), 1);
        entry.type = static_cast<AssetType>(type);
        f.read(reinterpret_cast<char*>(&entry.sizeBytes), 8);
        f.read(reinterpret_cast<char*>(&entry.contentHash), 8);
        entry.converterName = ReadString(f);
        entry.sourcePath = ReadString(f);
        entries_.push_back(std::move(entry));
    }

    RebuildIndex();
    return true;
}

bool AssetManifest::Save(const std::string& manifestPath) const {
    std::ofstream f(manifestPath, std::ios::binary);
    if (!f.is_open()) return false;

    WriteSchemaHeader(f, kManifestMagic, kManifestVersion);
    uint32_t count = static_cast<uint32_t>(entries_.size());
    f.write(reinterpret_cast<const char*>(&count), 4);

    for (const auto& entry : entries_) {
        WriteString(f, entry.path);
        uint8_t type = static_cast<uint8_t>(entry.type);
        f.write(reinterpret_cast<const char*>(&type), 1);
        f.write(reinterpret_cast<const char*>(&entry.sizeBytes), 8);
        f.write(reinterpret_cast<const char*>(&entry.contentHash), 8);
        WriteString(f, entry.converterName);
        WriteString(f, entry.sourcePath);
    }

    return f.good();
}

// -- Discovery -----------------------------------------------------

void AssetManifest::Scan(const std::string& directory) {
    entries_.clear();
    pathIndex_.clear();

    std::error_code ec;
    if (!fs::exists(directory, ec)) return;

    for (const auto& dirEntry : fs::recursive_directory_iterator(directory, ec)) {
        if (!dirEntry.is_regular_file()) continue;

        std::string relPath = fs::relative(dirEntry.path(), directory, ec).string();
        std::string ext = dirEntry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        AssetType type = InferType(ext);

        AssetEntry entry;
        entry.path = relPath;
        entry.type = type;
        entry.sizeBytes = static_cast<size_t>(dirEntry.file_size(ec));
        entry.contentHash = HashFile(dirEntry.path().string());
        entries_.push_back(std::move(entry));
    }

    RebuildIndex();
}

void AssetManifest::AddOrUpdate(const AssetEntry& entry) {
    auto it = pathIndex_.find(entry.path);
    if (it != pathIndex_.end()) {
        entries_[it->second] = entry;
    } else {
        pathIndex_[entry.path] = entries_.size();
        entries_.push_back(entry);
    }
}

bool AssetManifest::Remove(const std::string& path) {
    auto it = pathIndex_.find(path);
    if (it == pathIndex_.end()) return false;

    size_t idx = it->second;
    // Swap with last for O(1) removal.
    if (idx < entries_.size() - 1) {
        entries_[idx] = std::move(entries_.back());
    }
    entries_.pop_back();
    RebuildIndex();
    return true;
}

// -- Query ---------------------------------------------------------

const AssetEntry* AssetManifest::Find(const std::string& path) const {
    auto it = pathIndex_.find(path);
    if (it == pathIndex_.end()) return nullptr;
    return &entries_[it->second];
}

std::vector<const AssetEntry*> AssetManifest::FindByType(AssetType type) const {
    std::vector<const AssetEntry*> result;
    for (const auto& entry : entries_) {
        if (entry.type == type) result.push_back(&entry);
    }
    return result;
}

size_t AssetManifest::TotalBytes() const {
    size_t total = 0;
    for (const auto& entry : entries_) total += entry.sizeBytes;
    return total;
}

void AssetManifest::Clear() {
    entries_.clear();
    pathIndex_.clear();
}

// -- Utility -------------------------------------------------------

AssetType AssetManifest::InferType(const std::string& extension) {
    if (extension == ".kmesh" || extension == ".obj")  return AssetType::Mesh;
    if (extension == ".ktex"  || extension == ".bmp" ||
        extension == ".png"   || extension == ".jpg")  return AssetType::Texture;
    if (extension == ".wav"   || extension == ".ogg" ||
        extension == ".mp3"   || extension == ".kaudio") return AssetType::Audio;
    if (extension == ".ks")                            return AssetType::Script;
    if (extension == ".ttf"   || extension == ".otf")  return AssetType::Font;
    if (extension == ".kml"   || extension == ".kss")  return AssetType::Markup;
    if (extension == ".kmat")                          return AssetType::Material;
    return AssetType::Generic;
}

uint64_t AssetManifest::HashFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return 0;

    // FNV-1a 64-bit hash.
    uint64_t hash = 14695981039346656037ULL;
    char buf[4096];
    while (f.read(buf, sizeof(buf)) || f.gcount() > 0) {
        auto count = f.gcount();
        for (std::streamsize i = 0; i < count; ++i) {
            hash ^= static_cast<uint64_t>(static_cast<uint8_t>(buf[i]));
            hash *= 1099511628211ULL;
        }
    }
    return hash;
}

void AssetManifest::RebuildIndex() {
    pathIndex_.clear();
    for (size_t i = 0; i < entries_.size(); ++i) {
        pathIndex_[entries_[i].path] = i;
    }
}

} // namespace koilo
