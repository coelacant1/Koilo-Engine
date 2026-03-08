// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <koilo/platform/iscriptfilereader.hpp>
#include <string>

namespace koilo {
namespace platform {

/**
 * @brief Desktop file reader using C++ fstream
 * 
 * Reads files from local filesystem. Works on Linux, macOS, Windows.
 */
class DesktopFileReader : public IScriptFileReader {
public:
    DesktopFileReader();
    
    bool Read(const char* filepath, std::string& outContent) override;
    bool Exists(const char* filepath) override;
    size_t GetFileSize(const char* filepath) override;
    const char* GetLastError() const override;
    
private:
    std::string lastError;
};

} // namespace platform
} // namespace koilo
