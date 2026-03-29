// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file scene_pass.hpp
 * @brief Standard render graph pass: 3D scene setup + mesh rendering.
 *
 * Adds two passes to the graph:
 *   - "scene_begin": opens the offscreen render pass, uploads matrices/lights
 *   - "scene_meshes": renders all scene meshes
 *
 * Use sky_pass.hpp and debug_pass.hpp to insert sky and debug line
 * rendering between scene_begin and scene_end.
 */
#pragma once

#include "render_graph.hpp"
#include "../pipeline/render_pipeline.hpp"

namespace koilo::rhi {

/// Begin the offscreen scene pass (viewport, matrices, lights).
inline void AddSceneBeginPass(RenderGraph& graph,
                              RenderPipeline* pipeline,
                              Scene* scene,
                              CameraBase* camera) {
    graph.AddPass("scene_begin",
                  {},
                  {"offscreen", "transform_ubo", "light_data"},
                  [pipeline, scene, camera]() {
                      pipeline->BeginScenePass(scene, camera);
                  });
}

/// Render all scene meshes (requires active scene pass).
inline void AddSceneMeshesPass(RenderGraph& graph,
                               RenderPipeline* pipeline) {
    graph.AddPass("scene_meshes",
                  {"transform_ubo", "light_data"},
                  {"offscreen"},
                  [pipeline]() { pipeline->RenderSceneMeshes(); });
}

/// End the offscreen scene pass.
inline void AddSceneEndPass(RenderGraph& graph,
                            RenderPipeline* pipeline) {
    graph.AddPass("scene_end",
                  {"offscreen"},
                  {},
                  [pipeline]() { pipeline->EndScenePass(); });
}

} // namespace koilo::rhi
