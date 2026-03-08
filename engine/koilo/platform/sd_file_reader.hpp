// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <koilo/platform/iscriptfilereader.hpp>

namespace koilo {
namespace platform {

#if defined(TEENSYDUINO) || defined(ESP32) || defined(ARDUINO)

/**
 * @brief SD card file reader for microcontrollers
 * 
 * Reads files from SD card. Works on Teensy, ESP32, Arduino.
 */
class SDFileReader : public IScriptFileReader {
public:
    SDFileReader();
    
    bool Read(const char* filepath, std::string& outContent) override;
    bool Exists(const char* filepath) override;
    size_t GetFileSize(const char* filepath) override;
    const char* GetLastError() const override;
    
private:
    char lastError[128];
};

#endif // TEENSYDUINO || ESP32 || ARDUINO

} // namespace platform
} // namespace koilo
