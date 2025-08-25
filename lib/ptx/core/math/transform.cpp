#include "transform.hpp"

Transform::Transform() 
    : baseRotation(1, 0, 0, 0), rotation(1, 0, 0, 0), position(0, 0, 0), scale(1, 1, 1), scaleRotationOffset(1, 0, 0, 0) {}

Transform::Transform(const Vector3D& eulerXYZS, const Vector3D& position, const Vector3D& scale) {
    this->rotation = Rotation(EulerAngles(eulerXYZS, EulerConstants::EulerOrderXYZS)).GetQuaternion();
    this->position = position;
    this->scale = scale;
}

Transform::Transform(const Quaternion& rotation, const Vector3D& position, const Vector3D& scale) {
    this->rotation = rotation;
    this->position = position;
    this->scale = scale;
}

Transform::Transform(const Vector3D& eulerXYZS, const Vector3D& position, const Vector3D& scale, const Vector3D& rotationOffset, const Vector3D& scaleOffset) {
    this->rotation = Rotation(EulerAngles(eulerXYZS, EulerConstants::EulerOrderXYZS)).GetQuaternion();
    this->position = position;
    this->scale = scale;
    this->rotationOffset = rotationOffset;
    this->scaleOffset = scaleOffset;
}

Transform::Transform(const Quaternion& rotation, const Vector3D& position, const Vector3D& scale, const Vector3D& rotationOffset, const Vector3D& scaleOffset) {
    this->rotation = rotation;
    this->position = position;
    this->scale = scale;
    this->rotationOffset = rotationOffset;
    this->scaleOffset = scaleOffset;
}

Transform::Transform(const Transform& transform) {
    this->baseRotation = transform.baseRotation;
    this->rotation = transform.rotation;
    this->position = transform.position;
    this->scale = transform.scale;
    this->scaleRotationOffset = transform.scaleRotationOffset;
    this->rotationOffset = transform.rotationOffset;
    this->scaleOffset = transform.scaleOffset;
}

void Transform::SetBaseRotation(const Quaternion& baseRotation) {
    this->baseRotation = baseRotation;
}

Quaternion Transform::GetBaseRotation() const {
    return baseRotation;
}

void Transform::SetRotation(const Quaternion& rotation) {
    this->rotation = rotation;
}

void Transform::SetRotation(const Vector3D& eulerXYZS) {
    this->rotation = Rotation(EulerAngles(eulerXYZS, EulerConstants::EulerOrderXYZS)).GetQuaternion();
}

Quaternion Transform::GetRotation() const {
    return rotation * baseRotation;
}

void Transform::SetPosition(const Vector3D& position) {
    this->position = position;
}

Vector3D Transform::GetPosition() const {
    return position;
}

void Transform::SetScale(const Vector3D& scale) {
    this->scale = scale;
}

Vector3D Transform::GetScale() const {
    return scale;
}

void Transform::SetScaleRotationOffset(const Quaternion& scaleRotationOffset) {
    this->scaleRotationOffset = scaleRotationOffset;
}

Quaternion Transform::GetScaleRotationOffset() const {
    return scaleRotationOffset;
}

void Transform::SetRotationOffset(const Vector3D& rotationOffset) {
    this->rotationOffset = rotationOffset;
}

Vector3D Transform::GetRotationOffset() const {
    return rotationOffset;
}

void Transform::SetScaleOffset(const Vector3D& scaleOffset) {
    this->scaleOffset = scaleOffset;
}

Vector3D Transform::GetScaleOffset() const {
    return scaleOffset;
}

void Transform::Rotate(const Vector3D& eulerXYZS) {
    this->rotation = this->rotation * Rotation(EulerAngles(eulerXYZS, EulerConstants::EulerOrderXYZS)).GetQuaternion();
}

void Transform::Rotate(const Quaternion& rotation) {
    this->rotation = this->rotation * rotation;
}

void Transform::Translate(const Vector3D& offset) {
    this->position = this->position + offset;
}

void Transform::Scale(const Vector3D& scale) {
    this->scale = this->scale * scale;
}

ptx::UString Transform::ToString(){
    return "[" + Rotation(this->rotation).GetEulerAngles(EulerConstants::EulerOrderXYZS).Angles.ToString() + " " + this->position.ToString() + " " + this->scale.ToString() + "]";
}
