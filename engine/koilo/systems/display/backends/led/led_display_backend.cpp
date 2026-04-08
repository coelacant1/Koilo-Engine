// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file led_display_backend.cpp
 * @brief Implementation of the LED display backend.
 *
 * @date 04/06/2026
 * @author Coela Can't
 */

#ifdef KL_HAVE_LED_VOLUME

#include <koilo/systems/display/backends/led/led_display_backend.hpp>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <iostream>

namespace koilo {

LEDDisplayBackend::LEDDisplayBackend(ILEDTransport* transport,
                                     const LEDDisplayConfig& config)
    : transport_(transport),
      config_(config),
      initialized_(false),
      frozen_(false),
      currentSlot_(0),
      sequenceNum_(0),
      frameCount_(0),
      droppedFrames_(0) {
    info_.width = config_.ledCount;
    info_.height = 1;
    info_.name = config_.name;
    info_.refreshRate = config_.refreshRate;
    info_.AddCapability(DisplayCapability::RGB888);
    info_.AddCapability(DisplayCapability::DoubleBuffering);

    std::memset(gammaTable_, 0, sizeof(gammaTable_));
    BuildGammaTable();
}

LEDDisplayBackend::~LEDDisplayBackend() {
    if (initialized_) {
        Shutdown();
    }
}

bool LEDDisplayBackend::Initialize() {
    if (!transport_) {
        std::cerr << "[LEDDisplay] No transport provided" << std::endl;
        return false;
    }
    if (config_.ledCount == 0) {
        std::cerr << "[LEDDisplay] LED count is zero" << std::endl;
        return false;
    }
    if (config_.ledCount > LEDFrameEncoder::kMaxLEDCount) {
        std::cerr << "[LEDDisplay] LED count exceeds protocol maximum ("
                  << LEDFrameEncoder::kMaxLEDCount << ")" << std::endl;
        return false;
    }

    correctedBuffer_.resize(static_cast<size_t>(config_.ledCount) * 3, 0);
    currentSlot_ = 0;
    sequenceNum_ = 0;
    frameCount_ = 0;
    droppedFrames_ = 0;
    initialized_ = true;
    return true;
}

void LEDDisplayBackend::Shutdown() {
    initialized_ = false;
    correctedBuffer_.clear();
}

bool LEDDisplayBackend::IsInitialized() const {
    return initialized_;
}

DisplayInfo LEDDisplayBackend::GetInfo() const {
    return info_;
}

bool LEDDisplayBackend::HasCapability(DisplayCapability cap) const {
    return info_.HasCapability(cap);
}

bool LEDDisplayBackend::Present(const Framebuffer& fb) {
    if (!initialized_ || !transport_) {
        return false;
    }

    if (frozen_) {
        return true;
    }

    if (!transport_->IsReady()) {
        ++droppedFrames_;
        return false;
    }

    const size_t ledBytes = static_cast<size_t>(config_.ledCount) * 3;
    const auto* src = static_cast<const uint8_t*>(fb.data);

    if (!src || fb.GetSizeBytes() < static_cast<uint32_t>(ledBytes)) {
        return false;
    }

    // Apply gamma correction + brightness via LUT
    for (size_t i = 0; i < ledBytes; ++i) {
        correctedBuffer_[i] = gammaTable_[src[i]];
    }

    // Encode into wire protocol frame
    auto frame = LEDFrameEncoder::BuildFrame(
        sequenceNum_, currentSlot_,
        correctedBuffer_.data(), config_.ledCount);

    if (frame.empty()) {
        return false;
    }

    // Send via transport
    bool ok = transport_->Send(frame.data(), frame.size());
    if (ok) {
        ++frameCount_;
        ++sequenceNum_;
        currentSlot_ ^= 1; // alternate 0/1
    }

    return ok;
}

void LEDDisplayBackend::WaitVSync() {
    // LED strips have no vsync; timing is controlled by the controller
}

bool LEDDisplayBackend::Clear() {
    if (!initialized_ || !transport_) {
        return false;
    }

    const size_t ledBytes = static_cast<size_t>(config_.ledCount) * 3;
    std::memset(correctedBuffer_.data(), 0, ledBytes);

    auto frame = LEDFrameEncoder::BuildFrame(
        sequenceNum_, currentSlot_,
        correctedBuffer_.data(), config_.ledCount);

    if (frame.empty()) {
        return false;
    }

    bool ok = transport_->Send(frame.data(), frame.size());
    if (ok) {
        ++frameCount_;
        ++sequenceNum_;
        currentSlot_ ^= 1;
    }
    return ok;
}

bool LEDDisplayBackend::SetRefreshRate(uint32_t hz) {
    config_.refreshRate = hz;
    info_.refreshRate = hz;
    return true;
}

bool LEDDisplayBackend::SetOrientation(Orientation /*orient*/) {
    return false; // LED strips do not support orientation
}

bool LEDDisplayBackend::SetBrightness(uint8_t brightness) {
    config_.brightness = brightness;
    BuildGammaTable();
    return true;
}

bool LEDDisplayBackend::SetVSyncEnabled(bool /*enabled*/) {
    return false; // No vsync for LED strips
}

void LEDDisplayBackend::SetGamma(float gamma) {
    config_.gamma = gamma;
    BuildGammaTable();
}

void LEDDisplayBackend::BuildGammaTable() {
    const float g = (config_.gamma > 0.0f) ? config_.gamma : 1.0f;
    const float b = config_.brightness / 255.0f;

    for (int i = 0; i < 256; ++i) {
        float normalized = static_cast<float>(i) / 255.0f;
        float corrected = std::pow(normalized, g) * b;
        int val = static_cast<int>(corrected * 255.0f + 0.5f);
        gammaTable_[i] = static_cast<uint8_t>(std::min(std::max(val, 0), 255));
    }
}

} // namespace koilo

#endif // KL_HAVE_LED_VOLUME
