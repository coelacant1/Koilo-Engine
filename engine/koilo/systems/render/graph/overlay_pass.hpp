// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file overlay_pass.hpp
 * @brief Standard render graph pass: Canvas2D / UI compositing.
 *
 * Composites all active Canvas2D instances onto the swapchain.
 * Runs within the swapchain render pass opened by the blit pass.
 */
#pragma once

#include "render_graph.hpp"
#include "../pipeline/render_pipeline.hpp"

namespace koilo::rhi {

inline void AddOverlayPass(RenderGraph& graph,
                           RenderPipeline* pipeline,
                           int screenW, int screenH) {
    graph.AddPass("overlay",
                  {},
                  {"swapchain"},
                  [pipeline, screenW, screenH]() {
                      pipeline->CompositeCanvasOverlays(screenW, screenH);
                  });
}

} // namespace koilo::rhi
