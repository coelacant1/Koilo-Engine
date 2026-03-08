// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file hub75backend.hpp
 * @brief HUB75 RGB LED matrix display backend for embedded systems.
 *
 * This backend supports HUB75 LED matrix panels commonly used in embedded
 * projects. These panels are driven by shift registers and require precise
 * timing for scanning and PWM brightness control.
 *
 * Supported panels:
 * - 64x32 (2048 pixels)
 * - 64x64 (4096 pixels)
 * - Chained panels (multiple in series)
 *
 * Platform support:
 * - Teensy 4.x (ARM Cortex-M7, FlexIO DMA)
 * - ESP32/ESP32-S3 (I2S DMA)
 * - Arduino (bit-banging, slow)
 *
 * @date 2025-12-11
 * @author Coela
 */

#pragma once

#include <koilo/systems/display/idisplaybackend.hpp>
#include <koilo/systems/scene/scenedata.hpp>
#include "../../../../registry/reflect_macros.hpp"

namespace koilo {

/**
 * @class HUB75Backend
 * @brief Display backend for HUB75 RGB LED matrix panels.
 */
class HUB75Backend : public IDisplayBackend {
public:
    explicit HUB75Backend(const HUB75Config& config);
    HUB75Backend(uint16_t width, uint16_t height, uint8_t chainLength = 1);
    ~HUB75Backend() override;
    
    // IDisplayBackend implementation
    bool Initialize() override;
    void Shutdown() override;
    bool IsInitialized() const override;
    DisplayInfo GetInfo() const override;
    bool HasCapability(DisplayCapability cap) const override;
    bool Present(const Framebuffer& fb) override;
    void WaitVSync() override;
    bool Clear() override;
    bool SetRefreshRate(uint32_t hz) override;
    bool SetOrientation(Orientation orient) override;
    bool SetBrightness(uint8_t brightness) override;
    bool SetVSyncEnabled(bool enabled) override;
    
    // HUB75-specific methods
    bool SetColorDepth(uint8_t depth);
    uint8_t GetColorDepth() const;
    void SetGammaCorrectionEnabled(bool enabled);
    bool IsGammaCorrectionEnabled() const;
    bool SetScanPattern(uint8_t pattern);
    const HUB75Config& GetConfig() const;
    
private:
    HUB75Config config_;
    bool initialized_;
    bool gammaCorrectionEnabled_;
    uint8_t* framebuffer_[2];
    uint8_t activeBuffer_;
    DisplayInfo displayInfo_;
    uint8_t gammaTable_[256];
    
    bool AllocateFramebuffers();
    void FreeFramebuffers();
    bool InitializeDMA();
    void ShutdownDMA();
    bool ConfigureGPIO();
    bool StartRefresh();
    void StopRefresh();
    uint8_t ApplyGamma(uint8_t value) const;
    void BuildGammaTable();
    void SwapBuffers();

    KL_BEGIN_FIELDS(HUB75Backend)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(HUB75Backend)
        KL_METHOD_AUTO(HUB75Backend, Shutdown, "Shutdown"),
        KL_METHOD_AUTO(HUB75Backend, IsInitialized, "Is initialized"),
        KL_METHOD_AUTO(HUB75Backend, GetInfo, "Get info"),
        KL_METHOD_AUTO(HUB75Backend, HasCapability, "Has capability"),
        KL_METHOD_AUTO(HUB75Backend, Present, "Present"),
        KL_METHOD_AUTO(HUB75Backend, WaitVSync, "Wait vsync"),
        KL_METHOD_AUTO(HUB75Backend, Clear, "Clear"),
        KL_METHOD_AUTO(HUB75Backend, SetRefreshRate, "Set refresh rate"),
        KL_METHOD_AUTO(HUB75Backend, SetOrientation, "Set orientation"),
        KL_METHOD_AUTO(HUB75Backend, SetBrightness, "Set brightness"),
        KL_METHOD_AUTO(HUB75Backend, SetVSyncEnabled, "Set vsync enabled"),
        KL_METHOD_AUTO(HUB75Backend, GetColorDepth, "Get color depth"),
        KL_METHOD_AUTO(HUB75Backend, SetGammaCorrectionEnabled, "Set gamma correction enabled"),
        KL_METHOD_AUTO(HUB75Backend, IsGammaCorrectionEnabled, "Is gamma correction enabled"),
        KL_METHOD_AUTO(HUB75Backend, SetScanPattern, "Set scan pattern"),
        KL_METHOD_AUTO(HUB75Backend, GetConfig, "Get config")
    KL_END_METHODS

    // Reflection disabled - causes template issues with default parameters
    // TODO: Fix reflection for default constructor parameters
    /*
    KL_BEGIN_DESCRIBE(HUB75Backend)
        KL_CTOR(HUB75Backend, const HUB75Config& config),
        KL_CTOR(HUB75Backend, uint16_t width, uint16_t height, uint8_t chainLength)
    KL_END_DESCRIBE(HUB75Backend)
    */

};

} // namespace koilo
