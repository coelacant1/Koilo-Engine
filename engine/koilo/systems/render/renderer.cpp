// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/render/renderer.hpp>


namespace koilo {

void koilo::RenderingEngine::Rasterize(Scene* scene, CameraManager* cameraManager) {
    if (!scene || !cameraManager) return;
    for (int i = 0; i < cameraManager->GetCameraCount(); i++) {
        Rasterizer::Rasterize(scene, cameraManager->GetCameras()[i]);
    }
}

void koilo::RenderingEngine::RayTrace(Scene* scene, CameraManager* cameraManager) {
    if (!scene || !cameraManager) return;
    for (int i = 0; i < cameraManager->GetCameraCount(); i++) {
        RayTracer::RayTrace(scene, cameraManager->GetCameras()[i]);
    }
}

} // namespace koilo
