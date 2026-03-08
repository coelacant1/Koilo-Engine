// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file displayinfo.hpp
 * @brief Display information and capabilities.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

/**
 * @enum DisplayCapability
 * @brief Capabilities that a display backend may support.
 */
enum class DisplayCapability : uint8_t {
    RGB888,              ///< Full 24-bit color
    RGB565,              ///< 16-bit color
    Grayscale,           ///< 8-bit grayscale
    Monochrome,          ///< 1-bit monochrome
    HardwareScaling,     ///< Hardware can scale framebuffer
    HardwareRotation,    ///< Hardware can rotate display
    GPUAcceleration,     ///< GPU-accelerated rendering
    DoubleBuffering,     ///< Double-buffered presentation
    VSync,               ///< Vertical sync support
    Backlight,           ///< Backlight brightness control
    TouchInput,          ///< Touch input support
    PartialUpdate,       ///< Supports partial screen updates
};

/**
 * @enum Orientation
 * @brief Display orientation modes.
 */
enum class Orientation : uint8_t {
    Portrait_0,          ///< 0 degrees (default)
    Landscape_90,        ///< 90 degrees clockwise
    Portrait_180,        ///< 180 degrees
    Landscape_270,       ///< 270 degrees clockwise
};

/**
 * @struct DisplayInfo
 * @brief Information about a display backend.
 */
struct DisplayInfo {
    uint32_t width;                               ///< Display width in pixels
    uint32_t height;                              ///< Display height in pixels
    uint32_t refreshRate;                         ///< Refresh rate in Hz (0 = unknown)
    std::string name;                             ///< Display name/description
    std::vector<DisplayCapability> capabilities;  ///< Supported capabilities
    
    /**
     * @brief Default constructor.
     */
    DisplayInfo()
        : width(0), height(0), refreshRate(0), name("Unknown") {}
    
    /**
     * @brief Constructor with parameters.
     */
    DisplayInfo(uint32_t w, uint32_t h, const std::string& name)
        : width(w), height(h), refreshRate(0), name(name) {}
    
    /**
     * @brief Check if display has a specific capability.
     */
    bool HasCapability(DisplayCapability cap) const {
        for (const auto& c : capabilities) {
            if (c == cap) return true;
        }
        return false;
    }
    
    /**
     * @brief Add a capability.
     */
    void AddCapability(DisplayCapability cap) {
        if (!HasCapability(cap)) {
            capabilities.push_back(cap);
        }
    }

    KL_BEGIN_FIELDS(DisplayInfo)
        KL_FIELD(DisplayInfo, width, "Width", 0, 65535),
        KL_FIELD(DisplayInfo, height, "Height", 0, 65535),
        KL_FIELD(DisplayInfo, refreshRate, "Refresh rate", 0, 1000),
        KL_FIELD(DisplayInfo, name, "Name", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(DisplayInfo)
        KL_METHOD_AUTO(DisplayInfo, HasCapability, "Has capability"),
        KL_METHOD_AUTO(DisplayInfo, AddCapability, "Add capability")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(DisplayInfo)
        KL_CTOR0(DisplayInfo),
        KL_CTOR(DisplayInfo, uint32_t, uint32_t, const std::string&)
    KL_END_DESCRIBE(DisplayInfo)
};

} // namespace koilo
