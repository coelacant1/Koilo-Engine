// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/render/graph/frame_composer.hpp>
#include <koilo/systems/render/graph/scene_pass.hpp>
#include <koilo/systems/render/graph/sky_pass.hpp>
#include <koilo/systems/render/graph/debug_pass.hpp>
#include <koilo/systems/render/graph/blit_pass.hpp>
#include <koilo/systems/render/graph/overlay_pass.hpp>
#include <koilo/systems/render/pipeline/render_pipeline.hpp>
#include <koilo/systems/ui/ui.hpp>
#include <algorithm>

namespace koilo {
namespace rhi {

// -- Registration ----------------------------------------------------------

void FrameComposer::RegisterProvider(const std::string& name,
                                     PassPhase phase,
                                     ProviderFn fn) {
    // Replace existing provider with the same name
    for (auto& p : providers_) {
        if (p.name == name) {
            p.phase = phase;
            p.fn    = std::move(fn);
            sorted_ = false;
            return;
        }
    }
    providers_.push_back({name, phase, std::move(fn), true});
    sorted_ = false;
}

void FrameComposer::UnregisterProvider(const std::string& name) {
    providers_.erase(
        std::remove_if(providers_.begin(), providers_.end(),
                        [&](const ProviderInfo& p) { return p.name == name; }),
        providers_.end());
}

void FrameComposer::SetProviderEnabled(const std::string& name, bool enabled) {
    for (auto& p : providers_) {
        if (p.name == name) { p.enabled = enabled; return; }
    }
}

bool FrameComposer::IsProviderEnabled(const std::string& name) const {
    for (auto& p : providers_) {
        if (p.name == name) return p.enabled;
    }
    return false;
}

// -- Standard passes -------------------------------------------------------

void FrameComposer::RegisterStandardPasses() {
    // Scene: begin, sky, meshes, debug, end
    RegisterProvider("scene", PassPhase::Scene,
        [](RenderGraph& g, const FrameContext& ctx) {
            if (!ctx.pipeline) return;
            AddSceneBeginPass(g, ctx.pipeline, ctx.scene, ctx.camera);
            AddSkyPass(g, ctx.pipeline);
            AddSceneMeshesPass(g, ctx.pipeline);
            AddDebugLinesPass(g, ctx.pipeline);
            AddSceneEndPass(g, ctx.pipeline);
        });

    // Compose: stage canvas uploads, blit offscreen to swapchain, draw overlays.
    // Canvas staging MUST run before any render pass is opened, since it may
    // record vkCmdCopyBufferToImage on the active frame command buffer.
    RegisterProvider("compose", PassPhase::Compose,
        [](RenderGraph& g, const FrameContext& ctx) {
            if (!ctx.pipeline) return;
            AddCanvasStagePass(g, ctx.pipeline, ctx.screenW, ctx.screenH);
            AddBlitPass(g, ctx.pipeline, ctx.screenW, ctx.screenH);
            AddOverlayPass(g, ctx.pipeline, ctx.screenW, ctx.screenH);
        });

    // UI: lazy-init RHI + render
    RegisterProvider("ui", PassPhase::Overlay,
        [](RenderGraph& g, const FrameContext& ctx) {
            if (!ctx.ui || !ctx.pipeline) return;
            auto* ui       = ctx.ui;
            auto* pipeline = ctx.pipeline;
            int w = ctx.screenW, h = ctx.screenH;
            g.AddPass("ui", {}, {"swapchain"},
                [ui, pipeline, w, h]() {
                    if (!ui->IsRHIInitialized()) {
                        auto* dev = pipeline->GetDevice();
                        bool isVk = pipeline->UsesTopLeftOrigin();
                        ui->InitializeRHI(dev, isVk);
                    }
                    if (ui->IsRHIInitialized()) {
                        ui->RenderRHI(w, h);
                    }
                });
        });
}

// -- Per-frame execution ---------------------------------------------------

bool FrameComposer::BuildAndExecute(const FrameContext& ctx) {
    if (!sorted_) SortProviders();

    lastGraph_.Clear();

    for (auto& p : providers_) {
        if (p.enabled && p.fn) {
            p.fn(lastGraph_, ctx);
        }
    }

    if (!lastGraph_.Compile()) return false;
    lastGraph_.Execute(gpuTiming_);
    return true;
}

// -- Console queries -------------------------------------------------------

std::vector<std::string> FrameComposer::GetExecutionOrder() const {
    return lastGraph_.GetExecutionOrder();
}

// -- Internal --------------------------------------------------------------

void FrameComposer::SortProviders() {
    std::stable_sort(providers_.begin(), providers_.end(),
        [](const ProviderInfo& a, const ProviderInfo& b) {
            return static_cast<uint8_t>(a.phase) < static_cast<uint8_t>(b.phase);
        });
    sorted_ = true;
}

} // namespace rhi
} // namespace koilo
