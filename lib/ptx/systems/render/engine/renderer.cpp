#include "renderer.hpp"

void RenderingEngine::Rasterize(Scene* scene, CameraManager* cameraManager) {
    for (int i = 0; i < cameraManager->GetCameraCount(); i++) {
        Rasterizer::Rasterize(scene, cameraManager->GetCameras()[i]);
    }
}

void RenderingEngine::RayTrace(Scene* scene, CameraManager* cameraManager) {
    for (int i = 0; i < cameraManager->GetCameraCount(); i++) {
        RayTracer::RayTrace(scene, cameraManager->GetCameras()[i]);
    }
}
