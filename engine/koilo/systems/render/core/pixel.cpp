// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/render/core/pixel.hpp>


namespace koilo {

koilo::Pixel::Pixel() {
    // Default constructor
}

koilo::Pixel::Pixel(const Vector2D* position) {
    this->position = position;
}

const Vector2D koilo::Pixel::GetPosition() {
    return *position;
}

void koilo::Pixel::SetUpPixel(Pixel* pixel) {
    this->up = pixel;
    this->upExists = true;
}

void koilo::Pixel::SetDownPixel(Pixel* pixel) {
    this->down = pixel;
    this->downExists = true;
}

void koilo::Pixel::SetLeftPixel(Pixel* pixel) {
    this->left = pixel;
    this->leftExists = true;
}

void koilo::Pixel::SetRightPixel(Pixel* pixel) {
    this->right = pixel;
    this->rightExists = true;
}

bool koilo::Pixel::HasUpPixel() {
    return upExists;
}

bool koilo::Pixel::HasDownPixel() {
    return downExists;
}

bool koilo::Pixel::HasLeftPixel() {
    return leftExists;
}

bool koilo::Pixel::HasRightPixel() {
    return rightExists;
}

Pixel* koilo::Pixel::GetUpPixel() {
    return up;
}

Pixel* koilo::Pixel::GetDownPixel() {
    return down;
}

Pixel* koilo::Pixel::GetLeftPixel() {
    return left;
}

Pixel* koilo::Pixel::GetRightPixel() {
    return right;
}

} // namespace koilo
