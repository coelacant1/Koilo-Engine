// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/scene/camera/cameralayout.hpp>


namespace koilo {

koilo::CameraLayout::CameraLayout(ForwardAxis forwardAxis, UpAxis upAxis) {
    this->forwardAxis = forwardAxis;
    this->upAxis = upAxis;

    CalculateTransform();
}

bool koilo::CameraLayout::VerifyTransform() {
    if (forwardAxis == XForward || forwardAxis == XNForward) {
        return !(upAxis == XUp || upAxis == XNUp);
    } else if (forwardAxis == YForward || forwardAxis == YNForward) {
        return !(upAxis == YUp || upAxis == YNUp);
    } else {
        return !(upAxis == ZUp || upAxis == ZNUp);
    }
}

void koilo::CameraLayout::CalculateTransform() {
    Vector3D upVector, forwardVector, rightVector;

    if (VerifyTransform()) {
        upVector = GetUpVector();
        forwardVector = GetForwardVector();
        rightVector = upVector.CrossProduct(forwardVector);

        rotation = Rotation(rightVector, forwardVector, upVector).GetQuaternion().UnitQuaternion();
    }
    // else bad transform
}

koilo::CameraLayout::ForwardAxis koilo::CameraLayout::GetForwardAxis() {
    return forwardAxis;
}

koilo::CameraLayout::UpAxis koilo::CameraLayout::GetUpAxis() {
    return upAxis;
}

Vector3D koilo::CameraLayout::GetForwardVector() {
    Vector3D forwardVector;

    switch (forwardAxis) {
        case XForward: forwardVector.X = 1; break;
        case YForward: forwardVector.Y = 1; break;
        case XNForward: forwardVector.X = -1; break;
        case YNForward: forwardVector.Y = -1; break;
        case ZNForward: forwardVector.Z = -1; break;
        default: forwardVector.Z = 1; break;
    }

    return forwardVector;
}

Vector3D koilo::CameraLayout::GetUpVector() {
    Vector3D upVector;

    switch (upAxis) {
        case XUp: upVector.X = 1; break;
        case ZUp: upVector.Z = 1; break;
        case XNUp: upVector.X = -1; break;
        case YNUp: upVector.Y = -1; break;
        case ZNUp: upVector.Z = -1; break;
        default: upVector.Y = 1; break;
    }

    return upVector;
}

Quaternion koilo::CameraLayout::GetRotation() {
    return rotation;
}

} // namespace koilo
