// spiralparams.hpp
#pragma once
#include <array>
#include <algorithm> // for std::copy_n

#include "../../../../core/color/rgbcolor.hpp"
#include "../../../../core/math/vector2d.hpp"

/**
 * @file spiralparams.hpp
 * @brief Parameter block for a spiral color shader.
 * @tparam N Number of colors used by the spiral.
 */

/**
 * @tparam N Number of colors in the spiral.
 * @brief Parameters controlling spiral color selection and transforms.
 *
 * - @ref colors holds the active palette used during shading.
 * - @ref baseColors is an optional non-owning palette for external updates.
 * - @ref positionOffset and @ref rotationOffset define translation/rotation centers.
 * - @ref width, @ref bend, and @ref rotationAngle shape the spiral and orientation.
 */
template <size_t N>
struct SpiralParams {
    RGBColor colors[N];            ///< Active colors used by the shader.
    const RGBColor* baseColors{};  ///< Optional external palette (non-owning).

    Vector2D positionOffset{};     ///< XY shift applied after rotation transform.
    Vector2D rotationOffset{};     ///< Rotation origin (about Z).

    float    width{1.0f};          ///< Frequency of spiral turns.
    float    bend{0.0f};           ///< Radial bend factor.
    float    rotationAngle{0.0f};  ///< Global Z rotation in degrees.
    
    /**
     * @brief Construct from a fixed-size C-array palette.
     * @param pal      Source palette with N entries.
     * @param widthIn  Spiral frequency.
     * @param bendIn   Radial bend factor.
     * @param rotDeg   Rotation angle (degrees).
     * @param posOff   Translation offset.
     * @param rotOff   Rotation origin.
     * @param base     Optional external palette pointer (non-owning).
     */
    explicit SpiralParams(const RGBColor (&pal)[N],
                          float widthIn = 1.0f,
                          float bendIn = 0.0f,
                          float rotDeg = 0.0f,
                          Vector2D posOff = {},
                          Vector2D rotOff = {},
                          const RGBColor* base = nullptr) :
    baseColors(base),
    positionOffset(posOff),
    rotationOffset(rotOff),
    width(widthIn),
    bend(bendIn),
    rotationAngle(rotDeg) {
        std::copy_n(pal, N, colors);
    }

    /**
     * @brief Construct from std::array palette.
     * @param pal      Source palette with N entries.
     * @param widthIn  Spiral frequency.
     * @param bendIn   Radial bend factor.
     * @param rotDeg   Rotation angle (degrees).
     * @param posOff   Translation offset.
     * @param rotOff   Rotation origin.
     * @param base     Optional external palette pointer (non-owning).
     */
    explicit SpiralParams(const std::array<RGBColor, N>& pal,
                          float widthIn = 1.0f,
                          float bendIn = 0.0f,
                          float rotDeg = 0.0f,
                          Vector2D posOff = {},
                          Vector2D rotOff = {},
                          const RGBColor* base = nullptr) :
    baseColors(base),
    positionOffset(posOff),
    rotationOffset(rotOff),
    width(widthIn),
    bend(bendIn),
    rotationAngle(rotDeg) {
        std::copy_n(pal.data(), N, colors);
    }

    /** @brief Default constructor for aggregate initialization. */
    SpiralParams() = default;
};
