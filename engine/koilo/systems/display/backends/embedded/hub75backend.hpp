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

namespace koilo {

/**
 * @struct HUB75Config
 * @brief Configuration for HUB75 RGB LED matrix displays.
 */
struct HUB75Config {
    uint8_t r1_pin;      ///< Red data for upper half
    uint8_t g1_pin;      ///< Green data for upper half
    uint8_t b1_pin;      ///< Blue data for upper half
    uint8_t r2_pin;      ///< Red data for lower half
    uint8_t g2_pin;      ///< Green data for lower half
    uint8_t b2_pin;      ///< Blue data for lower half

    uint8_t a_pin;       ///< Row address A
    uint8_t b_pin;       ///< Row address B
    uint8_t c_pin;       ///< Row address C
    uint8_t d_pin;       ///< Row address D (for 32+ row panels)
    uint8_t e_pin;       ///< Row address E (for 64+ row panels)

    uint8_t clk_pin;     ///< Clock signal
    uint8_t lat_pin;     ///< Latch signal
    uint8_t oe_pin;      ///< Output enable (active low)

    uint8_t panelWidth;   ///< Width of single panel (typically 64)
    uint8_t panelHeight;  ///< Height of single panel (typically 32 or 64)
    uint8_t chainLength;  ///< Number of panels chained together
    uint8_t scanPattern;  ///< Scan pattern (0=1/16, 1=1/8, 2=1/4)

    uint8_t brightness;   ///< Global brightness (0-255)
    uint8_t colorDepth;   ///< Bits per color channel (8, 10, 12)
    uint8_t refreshRate;  ///< Target refresh rate in Hz
    uint8_t _pad;

    HUB75Config()
        : r1_pin(2), g1_pin(3), b1_pin(4),
          r2_pin(5), g2_pin(6), b2_pin(7),
          a_pin(8), b_pin(9), c_pin(10), d_pin(11), e_pin(12),
          clk_pin(13), lat_pin(14), oe_pin(15),
          panelWidth(64), panelHeight(32), chainLength(1), scanPattern(0),
          brightness(128), colorDepth(8), refreshRate(60), _pad(0) {}

} __attribute__((packed));

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
};

} // namespace koilo
