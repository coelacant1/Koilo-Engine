// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/platform/desktop_file_reader.hpp>
#include <fstream>
#include <sstream>

namespace koilo {
namespace platform {

DesktopFileReader::DesktopFileReader() : lastError("") {}

bool DesktopFileReader::Read(const char* filepath, std::string& outContent) {
        std::ifstream file(filepath, std::ios::in | std::ios::binary);
        
        if (!file.is_open()) {
            lastError = "Failed to open file: ";
            lastError += filepath;
            return false;
        }
        
        // Read entire file into string
        std::stringstream buffer;
        buffer << file.rdbuf();
        outContent = buffer.str();
        
        file.close();
        
        if (outContent.empty()) {
            lastError = "File is empty: ";
            lastError += filepath;
            return false;
        }
        
        lastError = "";
        return true;
}

bool DesktopFileReader::Exists(const char* filepath) {
        std::ifstream file(filepath);
        bool exists = file.good();
        file.close();
        return exists;
}

size_t DesktopFileReader::GetFileSize(const char* filepath) {
        std::ifstream file(filepath, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            return 0;
        }
        
        size_t size = static_cast<size_t>(file.tellg());
        file.close();
        return size;
}

const char* DesktopFileReader::GetLastError() const {
    return lastError.c_str();
}

} // namespace platform
} // namespace koilo
