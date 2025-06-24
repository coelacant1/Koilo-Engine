#include "../../../include/um3d/render/core/cameramanager.hpp"

CameraManager::CameraManager(CameraBase** cameras, uint8_t count) {
    this->cameras = cameras;
    this->count = count;
}

CameraBase** CameraManager::GetCameras() {
    return cameras;
}

uint8_t CameraManager::GetCameraCount() {
    return count;
}
