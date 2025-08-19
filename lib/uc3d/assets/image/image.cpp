// image.cpp
#include "image.hpp"

/**
 * @file image.cpp
 * @brief Implementation for palette-indexed Image with transform-aware sampling.
 * @date 8/18/2025
 * @author Coela Can't
 */

Image::Image(const uint8_t* data,
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

void Image::SetData(const uint8_t* data) {
    this->data = data;
}

void Image::SetColorPalette(const uint8_t* rgbColors) {
    this->rgbColors = rgbColors;
}

RGBColor Image::GetColorAtCoordinate(Vector2D point) {
    // Apply inverse rotation about 'offset' so we can do axis-aligned mapping.
    Vector2D rPos = angle != 0.0f ? point.Rotate(angle, offset) - offset : point - offset;

    // Map world-space to pixel-space (origin top-left, y increases downward).
    unsigned int x = (unsigned int)Mathematics::Map(rPos.X, size.X / -2.0f, size.X / 2.0f, float(xPixels), 0.0f);
    unsigned int y = (unsigned int)Mathematics::Map(rPos.Y, size.Y / -2.0f, size.Y / 2.0f, float(yPixels), 0.0f);

    // Outside? Return default color (0,0,0).
    if (x <= 1 || x >= xPixels || y <= 1 || y >= yPixels) return RGBColor();

    // Palette lookup: index -> triplet position
    unsigned int pos = data[x + y * xPixels] * 3;

    // Guard against invalid palette indices.
    if (pos > colors - (unsigned int)1) return RGBColor();

    return RGBColor(rgbColors[pos], rgbColors[pos + 1], rgbColors[pos + 2]);
}
