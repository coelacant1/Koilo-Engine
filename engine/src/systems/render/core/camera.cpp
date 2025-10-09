#include <ptx/systems/render/core/camera.hpp>

#include <ptx/systems/render/core/ipixelgroup.hpp>
#include <ptx/core/math/vector2d.hpp>
#include <ptx/core/math/vector3d.hpp>

Camera::Camera(Transform* transform, IPixelGroup* pixelGroup) {
    this->transform = transform;
    this->pixelGroup = pixelGroup;

    Set2D(true);
}

Camera::Camera(Transform* transform, CameraLayout* cameraLayout, IPixelGroup* pixelGroup) {
    this->transform = transform;
    this->pixelGroup = pixelGroup;
    this->cameraLayout = cameraLayout;

    if (transform != nullptr && cameraLayout != nullptr) {
        transform->SetBaseRotation(cameraLayout->GetRotation());
    }

    Set2D(false);
}

IPixelGroup* Camera::GetPixelGroup() {
    return pixelGroup;
}

Vector2D Camera::GetCameraMinCoordinate() {
    if (!pixelGroup) {
        return {};
    }

    if (!calculatedMin) {
        const uint16_t count = pixelGroup->GetPixelCount();
        for (uint16_t i = 0; i < count; ++i) {
            const Vector2D coordinate = pixelGroup->GetCoordinate(i);
            minC.X = coordinate.X < minC.X ? coordinate.X : minC.X;
            minC.Y = coordinate.Y < minC.Y ? coordinate.Y : minC.Y;
        }

        calculatedMin = true;
    }

    return minC;
}

Vector2D Camera::GetCameraMaxCoordinate() {
    if (!pixelGroup) {
        return {};
    }

    if (!calculatedMax) {
        const uint16_t count = pixelGroup->GetPixelCount();
        for (uint16_t i = 0; i < count; ++i) {
            const Vector2D coordinate = pixelGroup->GetCoordinate(i);
            maxC.X = coordinate.X > maxC.X ? coordinate.X : maxC.X;
            maxC.Y = coordinate.Y > maxC.Y ? coordinate.Y : maxC.Y;
        }

        calculatedMax = true;
    }

    return maxC;
}

Vector2D Camera::GetCameraCenterCoordinate() {
    return (GetCameraMinCoordinate() + GetCameraMaxCoordinate()) / 2.0f;
}

Vector3D Camera::GetCameraTransformMin() {
    Vector2D minV2 = GetCameraMinCoordinate();

    if (!transform) {
        return {};
    }

    return transform->GetRotation().RotateVector(Vector3D(minV2.X, minV2.Y, 0) * transform->GetScale()) + transform->GetPosition();
}

Vector3D Camera::GetCameraTransformMax() {
    Vector2D maxV2 = GetCameraMaxCoordinate();

    if (!transform) {
        return {};
    }

    return transform->GetRotation().RotateVector(Vector3D(maxV2.X, maxV2.Y, 0) * transform->GetScale()) + transform->GetPosition();
}

Vector3D Camera::GetCameraTransformCenter() {
    return (GetCameraTransformMin() + GetCameraTransformMax()) / 2.0f;
}
