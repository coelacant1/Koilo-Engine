// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/scene/camera/cameramanager.hpp>


namespace koilo {

koilo::CameraManager::CameraManager(CameraBase** cameras, uint8_t count) {
    this->cameras = cameras;
    this->count = count;
}

CameraBase** koilo::CameraManager::GetCameras() {
    return cameras;
}

uint8_t koilo::CameraManager::GetCameraCount() {
    return count;
}

CameraBase* koilo::CameraManager::GetCamera(uint8_t index) {
    if (index >= count || !cameras) return nullptr;
    return cameras[index];
}

} // namespace koilo
