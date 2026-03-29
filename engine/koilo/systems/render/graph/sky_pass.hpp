// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file sky_pass.hpp
 * @brief Standard render graph pass: sky / environment rendering.
 *
 * Renders the sky material into the offscreen target.  Must be added
 * between scene_begin and scene_end in the graph.
 */
#pragma once

#include "render_graph.hpp"
#include "../pipeline/render_pipeline.hpp"

namespace koilo::rhi {

inline void AddSkyPass(RenderGraph& graph,
                       RenderPipeline* pipeline) {
    graph.AddPass("sky",
                  {"transform_ubo"},
                  {"offscreen"},
                  [pipeline]() { pipeline->RenderSky(); });
}

} // namespace koilo::rhi
