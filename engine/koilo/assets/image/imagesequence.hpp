// SPDX-License-Identifier: GPL-3.0-or-later
// imagesequence.hpp
/**
 * @file ImageSequence.h
 * @brief Frame-sequenced animation driven by a palette-indexed Image.
 *
 * ImageSequence advances through an array of equally sized frames (pixel-index buffers)
 * at a target FPS. Each frame is assigned to a shared @ref Image, which owns the palette
 * and transform; the sequence only swaps the @ref Image::data pointer for the current frame.
 *
 * @author Coela Can't
 * @date 22/12/2024
 */
#pragma once

#include <stdint.h>
#include <koilo/core/math/vector2d.hpp>     // Vector2D
#include <koilo/core/math/mathematics.hpp>  // Mathematics::Map
#include <koilo/core/platform/time.hpp>     // koilo::Time::Millis
#include <koilo/core/color/color888.hpp>
#include "image.hpp"
#include <koilo/registry/reflect_macros.hpp>


namespace koilo {

/**
 * @class ImageSequence
 * @brief Cycles through multiple palette-indexed frames on a shared Image.
 *
 * Lifetime/ownership:
 * - Does not own @ref image or @ref data; caller manages memory.
 * - All frames must be width*height in size and compatible with @ref image’s palette.
 */
class ImageSequence {
public:
    /**
     * @brief Construct a sequence bound to an Image.
     * @param image Target image whose pixel data pointer will be swapped per frame.
     * @param data Array of frame pointers; each points to width*height bytes of indices.
     * @param imageCount Number of frames in @p data.
     * @param fps Frames per second for playback (can be changed at runtime).
     */
    ImageSequence(Image* image, const uint8_t** data, unsigned int imageCount, float fps);

    /** @brief Set target frames per second. */
    void SetFPS(float fps);

    /** @brief Pass-through to underlying Image size in world units. */
    void SetSize(Vector2D size);

    /** @brief Pass-through to underlying Image offset (rotation origin). */
    void SetPosition(Vector2D offset);

    /** @brief Pass-through to underlying Image rotation about its offset. */
    void SetRotation(float angle);

    /**
     * @brief Restart playback from the first frame and reset the internal clock.
     */
    void Reset();

    /**
     * @brief Advance frame based on elapsed time; updates the Image’s data pointer.
     * @note Call once per tick; uses koilo::Time::Millis() for scheduling.
     */
    void Update();

    /**
     * @brief Sample the current frame’s color at a coordinate (delegates to Image).
     */
    Color888 GetColorAtCoordinate(Vector2D point);

private:
    Image* image = nullptr;             ///< Target image (not owned)
    const uint8_t** data = nullptr;     ///< Frame array (not owned)
    unsigned int imageCount = 0;        ///< Number of frames
    float fps = 0.0f;                   ///< Target frames per second
    float frameTime = 0.0f;             ///< Duration of one full loop in seconds

    unsigned int currentFrame = 0;      ///< Current frame index
    uint64_t startTime = 0;             ///< Start timestamp (ms)

    KL_BEGIN_FIELDS(ImageSequence)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(ImageSequence)
        KL_METHOD_AUTO(ImageSequence, SetFPS, "Set fps"),
        KL_METHOD_AUTO(ImageSequence, SetSize, "Set size"),
        KL_METHOD_AUTO(ImageSequence, SetPosition, "Set position"),
        KL_METHOD_AUTO(ImageSequence, SetRotation, "Set rotation"),
        KL_METHOD_AUTO(ImageSequence, Reset, "Reset"),
        KL_METHOD_AUTO(ImageSequence, Update, "Update"),
        KL_METHOD_AUTO(ImageSequence, GetColorAtCoordinate, "Get color at coordinate")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(ImageSequence)
        KL_CTOR(ImageSequence, Image *, const uint8_t **, unsigned int, float)
    KL_END_DESCRIBE(ImageSequence)

};

} // namespace koilo
