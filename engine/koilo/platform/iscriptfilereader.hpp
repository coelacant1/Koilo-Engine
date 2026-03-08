// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstddef>
#include <string>

namespace koilo {
namespace platform {

/**
 * @brief Platform-agnostic interface for reading script files
 * 
 * This abstraction allows KoiloScript engine to work across platforms:
 * - Desktop: reads from filesystem using fstream
 * - Microcontroller: reads from SD card using SD.h
 * 
 * Usage:
 *   IScriptFileReader* reader = CreatePlatformFileReader();
 *   std::string content;
 *   if (reader->Read("animation.ks", content)) {
 *       // Parse and execute script
 *   }
 */
class IScriptFileReader {
public:
    virtual ~IScriptFileReader() = default;
    
    /**
     * @brief Read entire file contents as string
     * @param filepath Path to file (platform-specific format)
     * @param outContent Output string to store file contents
     * @return true if file read successfully, false otherwise
     */
    virtual bool Read(const char* filepath, std::string& outContent) = 0;
    
    /**
     * @brief Check if file exists
     * @param filepath Path to file
     * @return true if file exists and is accessible
     */
    virtual bool Exists(const char* filepath) = 0;
    
    /**
     * @brief Get file size in bytes
     * @param filepath Path to file
     * @return File size in bytes, or 0 if file doesn't exist
     */
    virtual size_t GetFileSize(const char* filepath) = 0;
    
    /**
     * @brief Get last error message
     * @return Human-readable error description
     */
    virtual const char* GetLastError() const = 0;
};

/**
 * @brief Factory function to create platform-specific file reader
 * 
 * Automatically selects implementation based on compile-time platform:
 * - KL_PLATFORM_DESKTOP -> DesktopFileReader (fstream)
 * - KL_PLATFORM_TEENSY40 -> SDFileReader (SD.h)
 * - KL_PLATFORM_ESP32 -> SDFileReader (SD.h)
 * 
 * @return Heap-allocated file reader (caller must delete)
 */
IScriptFileReader* CreatePlatformFileReader();

} // namespace platform
} // namespace koilo
