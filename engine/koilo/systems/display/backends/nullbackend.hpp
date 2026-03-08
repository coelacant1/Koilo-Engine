// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file nullbackend.hpp
 * @brief Null display backend for testing and headless operation.
 *
 * @date 11/10/2025
 * @author Coela
 */

#pragma once

#include <koilo/systems/display/idisplaybackend.hpp>

namespace koilo {

/**
 * @class NullBackend
 * @brief Display backend that does nothing (for testing).
 *
 * This backend accepts all frames but doesn't display them anywhere.
 * Useful for:
 * - Headless rendering
 * - Unit testing
 * - Performance benchmarking
 * - Simulation mode
 */
class NullBackend : public IDisplayBackend {
public:
    /**
     * @brief Constructor.
     * @param width Display width.
     * @param height Display height.
     * @param name Display name.
     */
    NullBackend(uint32_t width = 800, uint32_t height = 600, 
                const std::string& name = "NullDisplay");
    
    /**
     * @brief Destructor.
     */
    ~NullBackend() override = default;
    
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
    
    // === Statistics ===
    
    /**
     * @brief Get number of frames presented.
     */
    uint64_t GetFrameCount() const { return frameCount_; }
    
    /**
     * @brief Reset frame counter.
     */
    void ResetFrameCount() { frameCount_ = 0; }

private:
    DisplayInfo info_;
    bool initialized_;
    bool vsyncEnabled_;
    uint64_t frameCount_;

};

} // namespace koilo
