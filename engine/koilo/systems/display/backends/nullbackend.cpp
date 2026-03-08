// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/display/backends/nullbackend.hpp>

namespace koilo {

koilo::NullBackend::NullBackend(uint32_t width, uint32_t height, const std::string& name)
    : initialized_(false), vsyncEnabled_(false), frameCount_(0) {
    info_.width = width;
    info_.height = height;
    info_.name = name;
    info_.refreshRate = 60;  // Simulated 60 Hz
    
    // Add all capabilities (it's null, so it can "support" anything)
    info_.AddCapability(DisplayCapability::RGB888);
    info_.AddCapability(DisplayCapability::RGB565);
    info_.AddCapability(DisplayCapability::Grayscale);
    info_.AddCapability(DisplayCapability::Monochrome);
    info_.AddCapability(DisplayCapability::HardwareScaling);
    info_.AddCapability(DisplayCapability::HardwareRotation);
    info_.AddCapability(DisplayCapability::DoubleBuffering);
    info_.AddCapability(DisplayCapability::VSync);
}

bool koilo::NullBackend::Initialize() {
    initialized_ = true;
    return true;
}

void koilo::NullBackend::Shutdown() {
    initialized_ = false;
}

bool koilo::NullBackend::IsInitialized() const {
    return initialized_;
}

DisplayInfo koilo::NullBackend::GetInfo() const {
    return info_;
}

bool koilo::NullBackend::HasCapability(DisplayCapability cap) const {
    return info_.HasCapability(cap);
}

bool koilo::NullBackend::Present(const Framebuffer& fb) {
    if (!initialized_) {
        return false;
    }
    
    // Do nothing, just count the frame
    ++frameCount_;
    
    // Simulate some work (optional, for realistic timing)
    // volatile uint32_t dummy = 0;
    // for (int i = 0; i < 100; ++i) { dummy += i; }
    
    return true;
}

void koilo::NullBackend::WaitVSync() {
    // Do nothing - no actual display to sync with
}

bool koilo::NullBackend::Clear() {
    return initialized_;
}

bool koilo::NullBackend::SetRefreshRate(uint32_t hz) {
    info_.refreshRate = hz;
    return true;
}

bool koilo::NullBackend::SetOrientation(Orientation orient) {
    // Accept any orientation
    (void)orient;
    return true;
}

bool koilo::NullBackend::SetBrightness(uint8_t brightness) {
    // Accept any brightness
    (void)brightness;
    return true;
}

bool koilo::NullBackend::SetVSyncEnabled(bool enabled) {
    vsyncEnabled_ = enabled;
    return true;
}

} // namespace koilo
