// SPDX-License-Identifier: GPL-3.0-or-later
// image.cpp
#include <koilo/assets/image/image.hpp>


namespace koilo {

/**
 * @file image.cpp
 * @brief Implementation for palette-indexed Image with transform-aware sampling.
 * @date 8/18/2025
 * @author Coela Can't
 */

koilo::Image::Image(const uint8_t* data,
             const uint8_t* rgbColors,
             unsigned int xPixels,
             unsigned int yPixels,
             uint8_t colors) {
    this->data = data;
    this->rgbColors = rgbColors;
    this->xPixels = xPixels;
    this->yPixels = yPixels;
    this->colors = colors;
}

void koilo::Image::SetData(const uint8_t* data) {
    this->data = data;
}

void koilo::Image::SetColorPalette(const uint8_t* rgbColors) {
    this->rgbColors = rgbColors;
}

koilo::Color888 koilo::Image::GetColorAtCoordinate(Vector2D point) {
    // Apply inverse rotation about 'offset' to do axis-aligned mapping.
    Vector2D rPos = angle != 0.0f ? point.Rotate(angle, offset) - offset : point - offset;

    // Map world-space to pixel-space (origin top-left, y increases downward).
    unsigned int x = (unsigned int)Mathematics::Map(rPos.X, size.X / -2.0f, size.X / 2.0f, float(xPixels), 0.0f);
    unsigned int y = (unsigned int)Mathematics::Map(rPos.Y, size.Y / -2.0f, size.Y / 2.0f, float(yPixels), 0.0f);

    // Outside? Return default color (0,0,0).
    if (x <= 1 || x >= xPixels || y <= 1 || y >= yPixels) return koilo::Color888();

    // Palette lookup: index -> triplet position
    unsigned int pos = data[x + y * xPixels] * 3;

    // Guard against invalid palette indices.
    if (pos > colors - (unsigned int)1) return koilo::Color888();

    return koilo::Color888(rgbColors[pos], rgbColors[pos + 1], rgbColors[pos + 2]);
}

} // namespace koilo
