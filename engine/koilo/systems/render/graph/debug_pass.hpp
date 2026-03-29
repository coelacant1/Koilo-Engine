// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file debug_pass.hpp
 * @brief Standard render graph pass: debug lines and shapes.
 *
 * Renders debug lines from DebugDraw into the offscreen target.
 * Must be added between scene_begin and scene_end in the graph.
 */
#pragma once

#include "render_graph.hpp"
#include "../pipeline/render_pipeline.hpp"

namespace koilo::rhi {

inline void AddDebugLinesPass(RenderGraph& graph,
                              RenderPipeline* pipeline) {
    graph.AddPass("debug_lines",
                  {"transform_ubo"},
                  {"offscreen"},
                  [pipeline]() { pipeline->RenderSceneDebugLines(); });
}

} // namespace koilo::rhi
