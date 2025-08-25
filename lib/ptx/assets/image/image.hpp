// image.hpp
/**
 * @file Image.h
 * @brief Palette-indexed 2D image with transform and color sampling helpers.
 *
 * The Image class represents a static bitmap whose pixels are indices into an RGB palette.
 * It supports a local 2D transform (size, offset, rotation) and can sample an RGBColor
 * at an XY coordinate in the same space. Rotation is around @ref offset.
 *
 * @date 22/12/2024
 * @author Coela Can't
 */
#pragma once

#include <stdint.h>
#include "../../core/math/vector2d.hpp"      // Vector2D
#include "../../core/math/mathematics.hpp"   // Mathematics::Map
#include "../../core/color/rgbcolor.hpp"     // RGBColor

/**
 * @class Image
 * @brief Palette-indexed bitmap with transform-aware sampling.
 *
 * Storage layout:
 * - @ref data : width*height bytes; each byte is an index into @ref rgbColors
 * - @ref rgbColors : a packed palette of size (@ref colors * 3) bytes (R,G,B triplets)
 *
 * Transform:
 * - @ref size   : logical width/height in world units (used by sampling)
 * - @ref offset : center of the image (rotation origin)
 * - @ref angle  : rotation in degrees (positive = CCW), applied around @ref offset
 */
class Image {
public:
    /**
     * @brief Construct an Image backed by external memory.
     * @param data Pointer to width*height bytes of palette indices.
     * @param rgbColors Pointer to palette data (R,G,B triplets), length = colors*3.
     * @param xPixels Image width in pixels.
     * @param yPixels Image height in pixels.
     * @param colors Number of palette entries (triplets) available.
     * @note Pointers are *not* owned; caller retains lifetime management.
     */
    Image(const uint8_t* data,
          const uint8_t* rgbColors,
          unsigned int xPixels,
          unsigned int yPixels,
          uint8_t colors);

    /** @brief Replace the pixel-index buffer (width/height unchanged). */
    void SetData(const uint8_t* data);

    /** @brief Replace the RGB palette pointer (triplet-packed). */
    void SetColorPalette(const uint8_t* rgbColors);

    /** @brief Set logical display size (world units used during sampling). */
    void SetSize(Vector2D size) { this->size = size; }

    /** @brief Set the image center (also used as rotation origin). */
    void SetPosition(Vector2D offset) { this->offset = offset; }

    /**
     * @brief Set rotation angle about @ref offset.
     * @param angle Degrees (CCW positive).
     */
    void SetRotation(float angle) { this->angle = angle; }

    /**
     * @brief Sample color at a world-space coordinate considering size/offset/rotation.
     * @param point XY coordinate in the same space as @ref size and @ref offset.
     * @return RGBColor at that coordinate; returns default-constructed RGBColor() if
     *         out-of-bounds or palette index is invalid.
     */
    RGBColor GetColorAtCoordinate(Vector2D point);

private:
    // Backing data (not owned)
    const uint8_t* data = nullptr;       ///< width*height indices into @ref rgbColors
    const uint8_t* rgbColors = nullptr;  ///< palette as [R,G,B, R,G,B, ...], length = colors*3

    // Dimensions / palette
    unsigned int xPixels = 0;
    unsigned int yPixels = 0;
    uint8_t colors = 0;                  ///< number of palette entries

    // Local transform
    Vector2D size{1.0f, 1.0f};           ///< logical width/height in world units
    Vector2D offset{0.0f, 0.0f};         ///< center/rotation origin
    float angle = 0.0f;                  ///< degrees CCW
};
