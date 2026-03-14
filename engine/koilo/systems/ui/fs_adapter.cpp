// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file fs_adapter.cpp
 * @brief File system adapter implementation.
 * @date 03/08/2026
 * @author Coela Can't
 */

#include "fs_adapter.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>

#if defined(_WIN32)
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

namespace koilo {
namespace ui {

// ============================================================================
// Map common file extensions to icons.
IconId IconForExtension(const char* ext) {
    if (!ext || ext[0] == '\0') return IconId::File;
    // Skip leading dot
    if (ext[0] == '.') ++ext;
    // Meshes
    if (std::strcmp(ext, "obj") == 0 || std::strcmp(ext, "fbx") == 0 ||
        std::strcmp(ext, "gltf") == 0 || std::strcmp(ext, "glb") == 0 ||
        std::strcmp(ext, "stl") == 0)
        return IconId::Mesh;
    // Textures / images
    if (std::strcmp(ext, "png") == 0 || std::strcmp(ext, "jpg") == 0 ||
        std::strcmp(ext, "jpeg") == 0 || std::strcmp(ext, "bmp") == 0 ||
        std::strcmp(ext, "tga") == 0 || std::strcmp(ext, "hdr") == 0)
        return IconId::Texture;
    // Materials
    if (std::strcmp(ext, "mtl") == 0 || std::strcmp(ext, "mat") == 0)
        return IconId::Material;
    // Scripts
    if (std::strcmp(ext, "ksl") == 0 || std::strcmp(ext, "lua") == 0 ||
        std::strcmp(ext, "py") == 0 || std::strcmp(ext, "js") == 0)
        return IconId::Script;
    // Markup
    if (std::strcmp(ext, "kml") == 0 || std::strcmp(ext, "kss") == 0 ||
        std::strcmp(ext, "xml") == 0 || std::strcmp(ext, "html") == 0)
        return IconId::File;
    // Scenes / projects
    if (std::strcmp(ext, "scene") == 0 || std::strcmp(ext, "proj") == 0)
        return IconId::Settings;
    return IconId::File;
}

// ============================================================================
// List directory entries, directories first, then alphabetical.
std::vector<FsEntry> ListDirectory(const std::string& dirPath) {
    std::vector<FsEntry> entries;

#if defined(_WIN32)
    std::string pattern = dirPath + "\\*";
    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(pattern.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) return entries;
    do {
        if (fd.cFileName[0] == '.') continue; // skip . and ..
        FsEntry e;
        e.name  = fd.cFileName;
        e.path  = dirPath + "\\" + fd.cFileName;
        e.isDir = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        e.size  = e.isDir ? 0 : (static_cast<uint64_t>(fd.nFileSizeHigh) << 32) | fd.nFileSizeLow;
        e.icon  = e.isDir ? IconId::Folder : IconForExtension(FileExtension(fd.cFileName));
        entries.push_back(std::move(e));
    } while (FindNextFileA(hFind, &fd));
    FindClose(hFind);
#else
    DIR* dir = opendir(dirPath.c_str());
    if (!dir) return entries;
    struct dirent* ent;
    while ((ent = readdir(dir)) != nullptr) {
        if (ent->d_name[0] == '.') continue; // skip hidden / . / ..
        FsEntry e;
        e.name = ent->d_name;
        e.path = dirPath + "/" + ent->d_name;
        struct stat st{};
        if (stat(e.path.c_str(), &st) == 0) {
            e.isDir = S_ISDIR(st.st_mode);
            e.size  = e.isDir ? 0 : static_cast<uint64_t>(st.st_size);
        }
        e.icon = e.isDir ? IconId::Folder : IconForExtension(FileExtension(ent->d_name));
        entries.push_back(std::move(e));
    }
    closedir(dir);
#endif

    // Sort: directories first, then alphabetical (case-insensitive)
    std::sort(entries.begin(), entries.end(), [](const FsEntry& a, const FsEntry& b) {
        if (a.isDir != b.isDir) return a.isDir > b.isDir; // dirs first
        // Case-insensitive compare
        const char* pa = a.name.c_str();
        const char* pb = b.name.c_str();
        while (*pa && *pb) {
            int ca = (*pa >= 'A' && *pa <= 'Z') ? *pa + 32 : *pa;
            int cb = (*pb >= 'A' && *pb <= 'Z') ? *pb + 32 : *pb;
            if (ca != cb) return ca < cb;
            ++pa; ++pb;
        }
        return *pa == '\0' && *pb != '\0';
    });

    return entries;
}

// ============================================================================
// Split a path into breadcrumb segments.
std::vector<std::string> SplitPath(const std::string& path) {
    std::vector<std::string> parts;
    std::string seg;
    for (char c : path) {
        if (c == '/' || c == '\\') {
            if (!seg.empty()) {
                parts.push_back(std::move(seg));
                seg.clear();
            }
        } else {
            seg += c;
        }
    }
    if (!seg.empty()) parts.push_back(std::move(seg));
    return parts;
}

// ============================================================================
// Rebuild an absolute path from breadcrumb segments up to index (inclusive).
std::string JoinPath(const std::vector<std::string>& parts, size_t upTo) {
    std::string result;
#if !defined(_WIN32)
    result += '/';
#endif
    for (size_t i = 0; i <= upTo && i < parts.size(); ++i) {
        result += parts[i];
        if (i < upTo) {
#if defined(_WIN32)
            result += '\\';
#else
            result += '/';
#endif
        }
    }
    return result;
}

// ============================================================================
// Format a file size in human-readable form.
std::string FormatSize(uint64_t bytes) {
    if (bytes == 0) return "";
    const char* units[] = {"B", "KB", "MB", "GB"};
    int u = 0;
    double sz = static_cast<double>(bytes);
    while (sz >= 1024.0 && u < 3) { sz /= 1024.0; ++u; }
    char buf[32];
    if (u == 0) std::snprintf(buf, sizeof(buf), "%llu B", (unsigned long long)bytes);
    else        std::snprintf(buf, sizeof(buf), "%.1f %s", sz, units[u]);
    return buf;
}

} // namespace ui
} // namespace koilo
