#include "../../../include/um3d/geometry/3d/cube.hpp"

Cube::Cube(Vector3D centerPosition, Vector3D objectSize) {
    this->centerPosition = centerPosition;
    this->minimum = centerPosition - objectSize / 2.0f;
    this->maximum = centerPosition + objectSize / 2.0f;
}

Vector3D Cube::GetPosition() {
    return centerPosition;
}

Vector3D Cube::GetSize() {
    return maximum - minimum;
}

Vector3D Cube::GetMaximum() {
    return maximum;
}

Vector3D Cube::GetMinimum() {
    return minimum;
}

void Cube::Update(float dT, Vector3D acceleration, Quaternion rotation) {
    Quaternion rotationChange = rotation.Multiply(previousRotation.MultiplicativeInverse());
    velocity = rotationChange.RotateVector(velocity) + acceleration * dT; // maintain momentum of existing velocity, apply local acceleration
    position = position + velocity * dT;

    previousRotation = rotation;
}

Vector3D Cube::IsIntersecting(Cube* bO) {
    Vector3D collision;

    collision.X = maximum.X >= bO->GetMinimum().X ? 0 : 1;
    collision.X = bO->GetMaximum().X >= minimum.X ? 0 : -1;
    collision.Y = maximum.Y >= bO->GetMinimum().Y ? 0 : 1;
    collision.Y = bO->GetMaximum().Y >= minimum.Y ? 0 : -1;
    collision.Z = maximum.Z >= bO->GetMinimum().Z ? 0 : 1;
    collision.Z = bO->GetMaximum().Z >= minimum.Z ? 0 : -1;

    return collision;
}
