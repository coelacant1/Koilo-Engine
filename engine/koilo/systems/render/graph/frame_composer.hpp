// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
/**
 * @file frame_composer.hpp
 * @brief Owns the per-frame render graph and manages pass providers.
 *
 * Modules register PassProviders at startup; each frame the composer
 * iterates all enabled providers (sorted by phase), populates a
 * RenderGraph, compiles, and executes it.
 */

#include <koilo/systems/render/graph/render_graph.hpp>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace koilo {

class Scene;
class CameraBase;
class UI;
class GPUTimingManager;

namespace rhi {

class RenderPipeline;

/// Ordering phases for pass providers.
enum class PassPhase : uint8_t {
    Scene   = 0,  ///< Scene setup, sky, meshes, debug lines, scene end
    Post    = 1,  ///< Post-processing (bloom, SSAO, etc.)
    Compose = 2,  ///< Blit offscreen to swapchain, canvas overlays
    Overlay = 3,  ///< UI, HUD, debug overlays
};

/// Per-frame context passed to every pass provider.
struct FrameContext {
    RenderPipeline* pipeline = nullptr;
    Scene*          scene    = nullptr;
    CameraBase*     camera   = nullptr;
    UI*             ui       = nullptr;
    int             screenW  = 0;
    int             screenH  = 0;
};

/// Manages pass providers and builds/executes the per-frame render graph.
class FrameComposer {
public:
    using ProviderFn = std::function<void(RenderGraph&, const FrameContext&)>;

    struct ProviderInfo {
        std::string name;
        PassPhase   phase   = PassPhase::Scene;
        ProviderFn  fn;
        bool        enabled = true;
    };

    /// Register a named pass provider at the given phase.
    void RegisterProvider(const std::string& name, PassPhase phase,
                          ProviderFn fn);

    /// Remove a previously registered provider.
    void UnregisterProvider(const std::string& name);

    /// Enable or disable a provider by name.
    void SetProviderEnabled(const std::string& name, bool enabled);

    /// Check whether a provider is enabled.
    bool IsProviderEnabled(const std::string& name) const;

    /// Register the built-in scene / compose / UI pass providers.
    void RegisterStandardPasses();

    /// Build the render graph from all enabled providers, compile, execute.
    /// Returns false if compilation fails (e.g. cycle detected).
    bool BuildAndExecute(const FrameContext& ctx);

    // -- Console queries ---------------------------------------------------

    /// Execution order of the last compiled graph (pass names).
    std::vector<std::string> GetExecutionOrder() const;

    /// Read-only access to the provider list.
    const std::vector<ProviderInfo>& GetProviders() const { return providers_; }

    /// Read-only access to the last compiled graph.
    const RenderGraph& GetLastGraph() const { return lastGraph_; }

    /// Set the GPU timing manager for per-pass instrumentation.
    /// Owned externally; may be null to disable GPU timing.
    void SetGPUTiming(GPUTimingManager* timing) { gpuTiming_ = timing; }
    GPUTimingManager* GetGPUTiming() const { return gpuTiming_; }

private:
    std::vector<ProviderInfo> providers_;
    RenderGraph               lastGraph_;
    bool                      sorted_ = false;
    GPUTimingManager*         gpuTiming_ = nullptr;

    void SortProviders();
};

} // namespace rhi
} // namespace koilo
