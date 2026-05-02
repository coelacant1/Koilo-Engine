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

/// Stage canvas overlay texture uploads into the active frame command
/// buffer.  Must run BEFORE any render pass is opened (vkCmdCopyBufferToImage
/// is forbidden inside a render pass).  Writes the virtual resource
/// "canvas_textures" so that AddOverlayPass (which reads it) is scheduled
/// after this pass.
inline void AddCanvasStagePass(RenderGraph& graph,
                               RenderPipeline* pipeline,
                               int screenW, int screenH) {
    graph.AddPass("canvas_stage",
                  {},
                  {"canvas_textures"},
                  [pipeline, screenW, screenH]() {
                      pipeline->StageCanvasOverlays(screenW, screenH);
                  });
}

inline void AddOverlayPass(RenderGraph& graph,
                           RenderPipeline* pipeline,
                           int screenW, int screenH) {
    graph.AddPass("overlay",
                  {"canvas_textures"},
                  {"swapchain"},
                  [pipeline, screenW, screenH]() {
                      pipeline->CompositeCanvasOverlays(screenW, screenH);
                  });
}

} // namespace koilo::rhi
