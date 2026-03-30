// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file gpu_timing.cpp
 * @brief GPUTimingManager implementation.
 */
#include "gpu_timing.hpp"
#include <koilo/systems/render/rhi/rhi_device.hpp>
#include <koilo/systems/render/rhi/rhi_caps.hpp>

namespace koilo {

void GPUTimingManager::BeginFrame(rhi::IRHIDevice* device) {
    device_ = device;
    if (!enabled_ || !device_) return;

    // Cache timestamp period on first use
    if (periodNs_ <= 0.0) {
        periodNs_ = device_->GetTimestampPeriod();
        if (periodNs_ <= 0.0) {
            enabled_ = false;
            return;
        }
    }

    // Read back the previous frame's timestamps
    if (prevQueryCount_ > 0 && !prevPasses_.empty()) {
        std::vector<uint64_t> raw(prevQueryCount_, 0);
        if (device_->ReadTimestamps(raw.data(), prevQueryCount_)) {
            resolved_.clear();
            totalMs_ = 0.0;
            for (const auto& ps : prevPasses_) {
                uint64_t begin = raw[ps.beginIdx];
                uint64_t end   = raw[ps.endIdx];
                double deltaNs = static_cast<double>(end - begin) * periodNs_;
                double deltaMs = deltaNs / 1'000'000.0;
                if (deltaMs < 0.0) deltaMs = 0.0;
                resolved_.push_back({ps.name, deltaMs});
                totalMs_ += deltaMs;
            }
        }
    }

    // Swap current - prev
    prevPasses_     = std::move(currentPasses_);
    prevQueryCount_ = nextQueryIdx_;

    currentPasses_.clear();
    nextQueryIdx_ = 0;

    // Tell the device to reset its query pool for this frame
    device_->ResetTimestamps(kMaxPasses * 2);
}

void GPUTimingManager::BeginPass(const std::string& name) {
    if (!enabled_ || !device_ || nextQueryIdx_ >= kMaxPasses * 2) return;

    PassSlot slot;
    slot.name     = name;
    slot.beginIdx = nextQueryIdx_++;
    slot.endIdx   = 0; // filled in by EndPass

    device_->WriteTimestamp(slot.beginIdx);
    currentPasses_.push_back(std::move(slot));
}

void GPUTimingManager::EndPass(const std::string& name) {
    if (!enabled_ || !device_ || nextQueryIdx_ >= kMaxPasses * 2) return;

    // Find the most recent pass with this name that hasn't been ended
    for (auto it = currentPasses_.rbegin(); it != currentPasses_.rend(); ++it) {
        if (it->name == name && it->endIdx == 0) {
            it->endIdx = nextQueryIdx_++;
            device_->WriteTimestamp(it->endIdx);
            return;
        }
    }
}

void GPUTimingManager::EndFrame() {
    // Reserved for future use (e.g. async readback scheduling)
}

} // namespace koilo
