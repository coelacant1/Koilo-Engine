// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file blit_pass.hpp
 * @brief Standard render graph pass: blit offscreen to swapchain.
 *
 * Opens the swapchain render pass and draws the offscreen color
 * texture as a fullscreen quad.  The swapchain render pass is left
 * open for subsequent overlay and UI passes.
 */
#pragma once

#include "render_graph.hpp"
#include "../pipeline/render_pipeline.hpp"

namespace koilo::rhi {

inline void AddBlitPass(RenderGraph& graph,
                        RenderPipeline* pipeline,
                        int screenW, int screenH) {
    graph.AddPass("blit",
                  {"offscreen"},
                  {"swapchain"},
                  [pipeline, screenW, screenH]() {
                      pipeline->BlitToScreen(screenW, screenH);
                  });
}

} // namespace koilo::rhi
