// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/core/math/transform.hpp>


namespace koilo {

koilo::Transform::Transform() 
    : baseRotation(1, 0, 0, 0), rotation(1, 0, 0, 0), position(0, 0, 0), scale(1, 1, 1) {}

koilo::Transform::Transform(const Vector3D& eulerXYZS, const Vector3D& position, const Vector3D& scale) {
    this->rotation = Rotation(EulerAngles(eulerXYZS, EulerConstants::EulerOrderXYZS)).GetQuaternion();
    this->position = position;
    this->scale = scale;
}

koilo::Transform::Transform(const Quaternion& rotation, const Vector3D& position, const Vector3D& scale) {
    this->rotation = rotation;
    this->position = position;
    this->scale = scale;
}

koilo::Transform::Transform(const Vector3D& eulerXYZS, const Vector3D& position, const Vector3D& scale, const Vector3D& rotationOffset, const Vector3D& scaleOffset) {
    this->rotation = Rotation(EulerAngles(eulerXYZS, EulerConstants::EulerOrderXYZS)).GetQuaternion();
    this->position = position;
    this->scale = scale;
    this->rotationOffset = rotationOffset;
    this->scaleOffset = scaleOffset;
}

koilo::Transform::Transform(const Quaternion& rotation, const Vector3D& position, const Vector3D& scale, const Vector3D& rotationOffset, const Vector3D& scaleOffset) {
    this->rotation = rotation;
    this->position = position;
    this->scale = scale;
    this->rotationOffset = rotationOffset;
    this->scaleOffset = scaleOffset;
}

koilo::Transform::Transform(const Transform& transform) {
    this->baseRotation = transform.baseRotation;
    this->rotation = transform.rotation;
    this->position = transform.position;
    this->scale = transform.scale;
    this->rotationOffset = transform.rotationOffset;
    this->scaleOffset = transform.scaleOffset;
}

void koilo::Transform::SetBaseRotation(const Quaternion& baseRotation) {
    this->baseRotation = baseRotation;
}

Quaternion koilo::Transform::GetBaseRotation() const {
    return baseRotation;
}

void koilo::Transform::SetRotation(const Quaternion& rotation) {
    this->rotation = rotation;
}

void koilo::Transform::SetRotation(const Vector3D& eulerXYZS) {
    this->rotation = Rotation(EulerAngles(eulerXYZS, EulerConstants::EulerOrderXYZS)).GetQuaternion();
}

Quaternion koilo::Transform::GetRotation() const {
    return rotation * baseRotation;
}

void koilo::Transform::SetPosition(const Vector3D& position) {
    this->position = position;
}

Vector3D koilo::Transform::GetPosition() const {
    return position;
}

void koilo::Transform::SetScale(const Vector3D& scale) {
    this->scale = scale;
}

Vector3D koilo::Transform::GetScale() const {
    return scale;
}

void koilo::Transform::SetOrigin(const Vector3D& origin) {
    this->rotationOffset = origin;
    this->scaleOffset = origin;
}

Vector3D koilo::Transform::GetOrigin() const {
    return rotationOffset;
}

void koilo::Transform::SetRotationOffset(const Vector3D& rotationOffset) {
    this->rotationOffset = rotationOffset;
}

Vector3D koilo::Transform::GetRotationOffset() const {
    return rotationOffset;
}

void koilo::Transform::SetScaleOffset(const Vector3D& scaleOffset) {
    this->scaleOffset = scaleOffset;
}

Vector3D koilo::Transform::GetScaleOffset() const {
    return scaleOffset;
}

void koilo::Transform::Rotate(const Vector3D& eulerXYZS) {
    this->rotation = this->rotation * Rotation(EulerAngles(eulerXYZS, EulerConstants::EulerOrderXYZS)).GetQuaternion();
}

void koilo::Transform::Rotate(const Quaternion& rotation) {
    this->rotation = this->rotation * rotation;
}

void koilo::Transform::Translate(const Vector3D& offset) {
    this->position = this->position + offset;
}

void koilo::Transform::Scale(const Vector3D& scale) {
    this->scale = this->scale * scale;
}

koilo::UString koilo::Transform::ToString(){
    return "[" + Rotation(this->rotation).GetEulerAngles(EulerConstants::EulerOrderXYZS).Angles.ToString() + " " + this->position.ToString() + " " + this->scale.ToString() + "]";
}

} // namespace koilo
