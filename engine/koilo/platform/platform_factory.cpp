// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/platform/iscriptfilereader.hpp>
#include <koilo/platform/desktop_file_reader.hpp>
#include <koilo/platform/sd_file_reader.hpp>

namespace koilo {
namespace platform {

IScriptFileReader* CreatePlatformFileReader() {
    #if defined(KL_PLATFORM_DESKTOP) || defined(__linux__) || defined(__APPLE__) || defined(_WIN32)
        // Desktop platforms: Use fstream
        return new DesktopFileReader();
    #elif defined(TEENSYDUINO) || defined(ESP32) || defined(ARDUINO)
        // Microcontroller platforms: Use SD card
        return new SDFileReader();
    #else
        #error "Unsupported platform for IScriptFileReader"
        return nullptr;
    #endif
}

} // namespace platform
} // namespace koilo
