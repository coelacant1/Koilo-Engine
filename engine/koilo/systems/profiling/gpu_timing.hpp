// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file gpu_timing.hpp
 * @brief GPU timing manager - wraps RHI timestamp queries to provide
 *        per-pass GPU timing results in milliseconds.
 *
 * Usage:
 *   1. Call BeginFrame() at the start of each frame.
 *   2. Call BeginPass(name) / EndPass(name) around each render pass.
 *   3. Call EndFrame() after all passes.
 *   4. Query GetPassTimings() for the PREVIOUS frame's results
 *      (GPU readback is one frame behind).
 *
 * @date 07/08/2026
 * @author Coela Can't
 */
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace koilo::rhi { class IRHIDevice; }

namespace koilo {

/// Per-pass GPU timing result.
struct GPUPassTiming {
    std::string name;    ///< Pass name
    double      ms;      ///< Duration in milliseconds
};

/// Manages RHI timestamp queries and resolves per-pass GPU timings.
///
/// Internally allocates two timestamp slots per pass (begin + end).
/// Results are available one frame after recording (GPU latency).
class GPUTimingManager {
public:
    GPUTimingManager() = default;

    /// Prepare for a new frame of timing.  Resets query slots and
    /// reads back the previous frame's results if available.
    void BeginFrame(rhi::IRHIDevice* device);

    /// Record a begin-timestamp for the named pass.
    void BeginPass(const std::string& name);

    /// Record an end-timestamp for the named pass.
    void EndPass(const std::string& name);

    /// Finalize the frame (no-op currently, reserved for future use).
    void EndFrame();

    /// Enable or disable GPU timing collection.
    void SetEnabled(bool enabled) { enabled_ = enabled; }
    bool IsEnabled() const { return enabled_; }

    /// Previous frame's per-pass timings (empty until second frame).
    const std::vector<GPUPassTiming>& GetPassTimings() const { return resolved_; }

    /// Previous frame's total GPU time in milliseconds.
    double GetTotalGPUTimeMs() const { return totalMs_; }

    /// Maximum number of passes we can time per frame.
    static constexpr uint32_t kMaxPasses = 32;

private:
    rhi::IRHIDevice* device_ = nullptr;
    bool             enabled_ = false;

    // Current frame recording
    struct PassSlot {
        std::string name;
        uint32_t    beginIdx = 0;
        uint32_t    endIdx   = 0;
    };
    std::vector<PassSlot> currentPasses_;
    uint32_t              nextQueryIdx_ = 0;

    // Previous frame's pass layout (for interpreting readback data)
    std::vector<PassSlot> prevPasses_;
    uint32_t              prevQueryCount_ = 0;

    // Resolved results
    std::vector<GPUPassTiming> resolved_;
    double                     totalMs_ = 0.0;

    // Timestamp period (cached from device)
    double periodNs_ = 0.0;
};

} // namespace koilo
