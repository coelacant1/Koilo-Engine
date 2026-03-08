// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file idisplaybackend.hpp
 * @brief Interface for display backend implementations.
 *
 * This interface defines the contract that all display backends must implement,
 * enabling support for various output devices from microcontroller displays to
 * GPU-accelerated windows.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include "framebuffer.hpp"
#include "displayinfo.hpp"
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

/**
 * @class IDisplayBackend
 * @brief Abstract interface for display output backends.
 *
 * This interface provides a uniform API for presenting rendered frames to
 * various display devices. Implementations can target:
 * - Desktop windows (OpenGL, Vulkan, DirectX)
 * - Shared memory IPC
 * - Linux framebuffer (/dev/fb0)
 * - Microcontroller displays (SPI, I2C)
 * - LED matrices (WS2812, APA102)
 *
 * Lifecycle:
 * 1. Construct backend with device-specific parameters
 * 2. Call Initialize() to set up hardware/resources
 * 3. Call Present() repeatedly to show frames
 * 4. Call Shutdown() when done
 */
class IDisplayBackend {
public:
    /**
     * @brief Virtual destructor.
     */
    virtual ~IDisplayBackend() = default;
    
    // === Lifecycle ===
    
    /**
     * @brief Initialize the display backend.
     * @return True if initialization succeeded, false otherwise.
     *
     * This should allocate resources, initialize hardware, and prepare
     * the backend for frame presentation.
     */
    virtual bool Initialize() = 0;
    
    /**
     * @brief Shutdown the display backend.
     *
     * This should release resources, close hardware connections, and
     * clean up any state.
     */
    virtual void Shutdown() = 0;
    
    /**
     * @brief Check if backend is initialized.
     * @return True if initialized and ready to present frames.
     */
    virtual bool IsInitialized() const = 0;
    
    // === Information ===
    
    /**
     * @brief Get display information.
     * @return DisplayInfo structure with metadata.
     */
    virtual DisplayInfo GetInfo() const = 0;
    
    /**
     * @brief Check if display has a specific capability.
     * @param cap Capability to check.
     * @return True if capability is supported.
     */
    virtual bool HasCapability(DisplayCapability cap) const = 0;
    
    // === Frame Presentation ===
    
    /**
     * @brief Present a framebuffer to the display.
     * @param fb Framebuffer containing pixel data to display.
     * @return True if presentation succeeded, false otherwise.
     *
     * This is the main rendering function. It should:
     * 1. Convert pixel format if needed
     * 2. Transfer data to display hardware
     * 3. Trigger display update
     *
     * Note: This function may block waiting for VSync if enabled.
     */
    virtual bool Present(const Framebuffer& fb) = 0;
    
    /**
     * @brief Wait for vertical sync.
     *
     * Blocks until the next VSync signal. No-op if VSync not supported.
     * Used to prevent tearing and limit frame rate.
     */
    virtual void WaitVSync() = 0;
    
    /**
     * @brief Clear the display to black (or off).
     * @return True if clear succeeded.
     */
    virtual bool Clear() = 0;
    
    // === Configuration ===
    
    /**
     * @brief Set refresh rate.
     * @param hz Desired refresh rate in Hz.
     * @return True if refresh rate was set, false if not supported.
     */
    virtual bool SetRefreshRate(uint32_t hz) = 0;
    
    /**
     * @brief Set display orientation.
     * @param orient Desired orientation.
     * @return True if orientation was set, false if not supported.
     */
    virtual bool SetOrientation(Orientation orient) = 0;
    
    /**
     * @brief Set backlight brightness (if supported).
     * @param brightness Brightness level (0-255).
     * @return True if backlight was set, false if not supported.
     */
    virtual bool SetBrightness(uint8_t brightness) = 0;
    
    /**
     * @brief Enable or disable VSync.
     * @param enabled True to enable VSync, false to disable.
     * @return True if VSync setting was changed.
     */
    virtual bool SetVSyncEnabled(bool enabled) = 0;

    // === Reflection Metadata ===
    
    KL_BEGIN_FIELDS(IDisplayBackend)
        /* No reflected fields - pure interface. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(IDisplayBackend)
        KL_METHOD_AUTO(IDisplayBackend, Initialize, "Initialize"),
        KL_METHOD_AUTO(IDisplayBackend, Shutdown, "Shutdown"),
        KL_METHOD_AUTO(IDisplayBackend, IsInitialized, "Is initialized"),
        KL_METHOD_AUTO(IDisplayBackend, GetInfo, "Get info"),
        KL_METHOD_AUTO(IDisplayBackend, HasCapability, "Has capability"),
        KL_METHOD_AUTO(IDisplayBackend, Present, "Present"),
        KL_METHOD_AUTO(IDisplayBackend, WaitVSync, "Wait vsync"),
        KL_METHOD_AUTO(IDisplayBackend, Clear, "Clear"),
        KL_METHOD_AUTO(IDisplayBackend, SetRefreshRate, "Set refresh rate"),
        KL_METHOD_AUTO(IDisplayBackend, SetOrientation, "Set orientation"),
        KL_METHOD_AUTO(IDisplayBackend, SetBrightness, "Set brightness"),
        KL_METHOD_AUTO(IDisplayBackend, SetVSyncEnabled, "Set vsync enabled")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(IDisplayBackend)
        /* No reflected ctors - abstract interface. */
    KL_END_DESCRIBE(IDisplayBackend)
};

/**
 * @class IGeometryDisplayBackend
 * @brief Optional interface for backends that support receiving per-pixel geometry.
 */
class IGeometryDisplayBackend {
public:
    virtual ~IGeometryDisplayBackend() = default;

    /**
     * @brief Update the XY geometry data for the next presentation.
     * @param xyPairs Pointer to an array of XY pairs (length = pixelCount * 2).
     * @param pixelCount Number of pixels represented in the geometry buffer.
     */
    virtual void UpdateGeometry(const float* xyPairs, uint32_t pixelCount) = 0;
};

} // namespace koilo
