// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/core/geometry/3d/cube.hpp>


namespace koilo {

koilo::Cube::Cube(Vector3D centerPosition, Vector3D objectSize) {
    this->centerPosition = centerPosition;
    this->minimum = centerPosition - objectSize / 2.0f;
    this->maximum = centerPosition + objectSize / 2.0f;
}

Vector3D koilo::Cube::GetPosition() {
    return centerPosition;
}

Vector3D koilo::Cube::GetSize() {
    return maximum - minimum;
}

Vector3D koilo::Cube::GetMaximum() {
    return maximum;
}

Vector3D koilo::Cube::GetMinimum() {
    return minimum;
}

} // namespace koilo
