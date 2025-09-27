#pragma once

#include "../core/ipixelgroup.hpp"
#include "../../../core/math/mathematics.hpp"

/**
 * @file effect.hpp
 * @brief Base post-process effect that operates on a camera's IPixelGroup.
 *
 * Typical call order per frame: @ref SetRatio -> @ref Apply.
 * The ratio is a normalized control value in [0,1] that implementations may
 * interpret as time, intensity, or blend factor.
 */

/**
 * @class Effect
 * @brief Abstract base class for post-processing effects on an IPixelGroup.
 *
 * Derived effects modify the pixel group's current color buffer in place.
 * Implementations should read from @c IPixelGroup::GetColors() and can use
 * @c IPixelGroup::GetColorBuffer() as temporary scratch storage.
 */
class Effect {
protected:
    float ratio = 0.0f;   ///< Normalized control value in [0,1].

public:
    /** @brief Virtual destructor. */
    virtual ~Effect() = default;

    /**
     * @brief Set the effect's normalized control ratio.
     * @param r Value in [0,1] (interpretation is effect-specific).
     */
    void SetRatio(float r);

    /**
     * @brief Apply the effect in-place to the pixel group's current color buffer.
     * @param pixelGroup Target pixel group; must remain valid for the duration of the call.
     *
     * Implementations should treat @c GetColors() as the source buffer and may use
     * @c GetColorBuffer() as a temporary scratch buffer when needed.
     */
    virtual void Apply(IPixelGroup* pixelGroup) = 0;

};
