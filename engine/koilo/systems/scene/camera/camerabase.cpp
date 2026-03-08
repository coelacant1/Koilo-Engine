// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/scene/camera/camerabase.hpp>


namespace koilo {

koilo::CameraBase::CameraBase() {}

CameraLayout* koilo::CameraBase::GetCameraLayout() {
    return cameraLayout;
}

Transform* koilo::CameraBase::GetTransform() {
    return transform;
}

bool koilo::CameraBase::Is2D() {
    return is2D;
}

void koilo::CameraBase::Set2D(bool is2D) {
    this->is2D = is2D;
}

void koilo::CameraBase::SetLookOffset(Quaternion lookOffset) {
    this->lookOffset = lookOffset;
}

Quaternion koilo::CameraBase::GetLookOffset() {
    return lookOffset;
}

void koilo::CameraBase::SetPerspective(float fov, float nearPlane, float farPlane) {
    projectionType_ = ProjectionType::PERSPECTIVE;
    fov_ = fov;
    nearPlane_ = nearPlane;
    farPlane_ = farPlane;
}

} // namespace koilo
