// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file fs_adapter.hpp
 * @brief File system adapter for the content browser - directory listing,
 *        icon mapping, and path utilities.
 *
 * @date 03/08/2026
 * @author Coela Can't
 */

#pragma once

#include <cstring>
#include <string>
#include <vector>
#include <koilo/systems/ui/icon_ids.hpp>

namespace koilo {
namespace ui {

/** @class FsEntry @brief A single file or directory entry. */
struct FsEntry {
    std::string name;      ///< File name (not full path)
    std::string path;      ///< Full absolute or relative path
    bool        isDir  = false; ///< True if this entry is a directory
    uint64_t    size   = 0;    ///< File size in bytes (0 for directories)
    IconId      icon   = IconId::File; ///< Icon representing the file type
};

/** @brief Map common file extensions to icons. */
IconId IconForExtension(const char* ext);

/** @brief Extract the extension from a filename (returns pointer into name, or ""). */
inline const char* FileExtension(const char* name) {
    const char* dot = std::strrchr(name, '.');
    return dot ? dot : "";
}

/** @brief List directory entries. Directories come first, then alphabetical. */
std::vector<FsEntry> ListDirectory(const std::string& dirPath);

/**
 * @brief Split a path into breadcrumb segments.
 *
 * e.g. "/home/user/assets/models" -> ["home", "user", "assets", "models"]
 */
std::vector<std::string> SplitPath(const std::string& path);

/** @brief Rebuild an absolute path from breadcrumb segments up to index (inclusive). */
std::string JoinPath(const std::vector<std::string>& parts, size_t upTo);

/** @brief Format a file size in human-readable form. */
std::string FormatSize(uint64_t bytes);

} // namespace ui
} // namespace koilo
