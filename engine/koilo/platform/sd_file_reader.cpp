// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/platform/sd_file_reader.hpp>

#if defined(TEENSYDUINO) || defined(ESP32) || defined(ARDUINO)

#include <SD.h>
#include <cstring>

namespace koilo {
namespace platform {

SDFileReader::SDFileReader() {
    lastError[0] = '\0';
}

bool SDFileReader::Read(const char* filepath, std::string& outContent) {
        // Check if SD card is initialized
        if (!SD.begin(BUILTIN_SDCARD)) {
            strncpy(lastError, "SD card not initialized", sizeof(lastError) - 1);
            return false;
        }
        
        File file = SD.open(filepath, FILE_READ);
        
        if (!file) {
            snprintf(lastError, sizeof(lastError), "Failed to open: %s", filepath);
            return false;
        }
        
        // Get file size
        size_t fileSize = file.size();
        
        if (fileSize == 0) {
            file.close();
            snprintf(lastError, sizeof(lastError), "File is empty: %s", filepath);
            return false;
        }
        
        // Allocate buffer and read
        outContent.reserve(fileSize + 1);
        outContent.clear();
        
        while (file.available()) {
            char c = file.read();
            outContent += c;
        }
        
        file.close();
        
        lastError[0] = '\0';
        return true;
}

bool SDFileReader::Exists(const char* filepath) {
        if (!SD.begin(BUILTIN_SDCARD)) {
            return false;
        }
        return SD.exists(filepath);
}

size_t SDFileReader::GetFileSize(const char* filepath) {
        if (!SD.begin(BUILTIN_SDCARD)) {
            return 0;
        }
        
        File file = SD.open(filepath, FILE_READ);
        if (!file) {
            return 0;
        }
        
        size_t size = file.size();
        file.close();
        return size;
}

const char* SDFileReader::GetLastError() const {
    return lastError;
}

} // namespace platform
} // namespace koilo

#endif // TEENSYDUINO || ESP32 || ARDUINO
