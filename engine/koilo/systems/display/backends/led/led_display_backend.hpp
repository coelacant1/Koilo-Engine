// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file led_display_backend.hpp
 * @brief Display backend that sends framebuffer data to an LED strip
 *        controller via an ILEDTransport link.
 *
 * The backend collects RGB pixel data from the engine framebuffer, encodes
 * it into the wire protocol defined by LEDFrameEncoder, and transmits it
 * through an injected ILEDTransport instance. It tracks double-buffer slot
 * state and drops frames when the transport is not ready.
 *
 * This file is conditionally compiled behind KL_HAVE_LED_VOLUME.
 *
 * @date 04/06/2026
 * @author Coela Can't
 */

#pragma once

#ifdef KL_HAVE_LED_VOLUME

#include <koilo/systems/display/idisplaybackend.hpp>
#include <koilo/systems/display/led/iled_transport.hpp>
#include <koilo/systems/display/led/led_frame_encoder.hpp>
#include <cstdint>
#include <vector>
#include <string>

namespace koilo {

/**
 * @struct LEDDisplayConfig
 * @brief Configuration for the LED display backend.
 */
struct LEDDisplayConfig {
    uint16_t ledCount;       ///< Total number of LEDs across all groups.
    uint8_t  brightness;     ///< Global brightness (0-255).
    float    gamma;          ///< Gamma correction exponent (e.g. 2.2).
    uint32_t refreshRate;    ///< Target refresh rate in Hz.
    std::string name;        ///< Display name.

    LEDDisplayConfig()
        : ledCount(0),
          brightness(255),
          gamma(2.2f),
          refreshRate(60),
          name("LEDDisplay") {}
};

/**
 * @class LEDDisplayBackend
 * @brief IDisplayBackend implementation for driving LED strips via ILEDTransport.
 *
 * The backend:
 * - Accepts a Framebuffer from the engine (typically RGB888)
 * - Applies gamma correction and brightness scaling
 * - Encodes the data into the wire protocol via LEDFrameEncoder
 * - Sends the encoded frame through the injected ILEDTransport
 * - Tracks double-buffer slot alternation (0/1)
 * - Drops frames when the transport is busy
 *
 * The transport is injected via the constructor and is NOT owned by this class.
 * Ownership of the transport resides with the module that created it.
 */
class LEDDisplayBackend : public IDisplayBackend {
public:
    /**
     * @brief Constructor.
     * @param transport Non-owning pointer to the transport. Must outlive this backend.
     * @param config Display configuration.
     */
    LEDDisplayBackend(ILEDTransport* transport, const LEDDisplayConfig& config);

    /**
     * @brief Destructor.
     */
    ~LEDDisplayBackend() override;

    // === IDisplayBackend Interface ===

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

    // === LED-specific accessors ===

    /**
     * @brief Get the total number of frames presented.
     * @return Frame count.
     */
    uint64_t GetFrameCount() const { return frameCount_; }

    /**
     * @brief Get the number of frames dropped due to transport busy.
     * @return Dropped frame count.
     */
    uint64_t GetDroppedFrames() const { return droppedFrames_; }

    /**
     * @brief Get the current gamma value.
     * @return Gamma exponent.
     */
    float GetGamma() const { return config_.gamma; }

    /**
     * @brief Set the gamma correction exponent and rebuild the LUT.
     * @param gamma Gamma value (typically 1.0 to 3.0).
     */
    void SetGamma(float gamma);

    /**
     * @brief Check if output is currently frozen (paused).
     * @return True if frozen.
     */
    bool IsFrozen() const { return frozen_; }

    /**
     * @brief Freeze or unfreeze frame output.
     * @param frozen True to freeze, false to resume.
     */
    void SetFrozen(bool frozen) { frozen_ = frozen; }

    /**
     * @brief Get the LED count.
     * @return Number of LEDs.
     */
    uint16_t GetLEDCount() const { return config_.ledCount; }

private:
    void BuildGammaTable();

    ILEDTransport*        transport_;
    LEDDisplayConfig      config_;
    DisplayInfo           info_;
    bool                  initialized_;
    bool                  frozen_;
    uint8_t               currentSlot_;
    uint8_t               sequenceNum_;
    uint64_t              frameCount_;
    uint64_t              droppedFrames_;
    uint8_t               gammaTable_[256];
    std::vector<uint8_t>  correctedBuffer_;
};

} // namespace koilo

#endif // KL_HAVE_LED_VOLUME
