// spiralparams.hpp
#pragma once
#include <vector>
#include <algorithm>

#include "../../../../core/color/rgbcolor.hpp"
#include "../../../../core/math/vector2d.hpp"
#include "../../../../registry/reflect_macros.hpp"

/**
 * @file spiralparams.hpp
 * @brief Parameter block for a spiral color shader with runtime-sized color array.
 */

/**
 * @brief Parameters controlling spiral color selection and transforms.
 *
 * - @ref colors holds the active palette used during shading (runtime-sized vector).
 * - @ref baseColors is an optional non-owning palette for external updates.
 * - @ref positionOffset and @ref rotationOffset define translation/rotation centers.
 * - @ref width, @ref bend, and @ref rotationAngle shape the spiral and orientation.
 */
struct SpiralParams {
    std::vector<RGBColor> colors;  ///< Active colors used by the shader (runtime-sized).
    const RGBColor* baseColors{};  ///< Optional external palette (non-owning).

    Vector2D positionOffset{};     ///< XY shift applied after rotation transform.
    Vector2D rotationOffset{};     ///< Rotation origin (about Z).

    float    width{1.0f};          ///< Frequency of spiral turns.
    float    bend{0.0f};           ///< Radial bend factor.
    float    rotationAngle{0.0f};  ///< Global Z rotation in degrees.
    
    /**
     * @brief Default constructor.
     */
    SpiralParams() = default;

    /**
     * @brief Construct with specified color count.
     * @param colorCount Number of colors to initialize.
     * @param widthIn    Spiral frequency.
     * @param bendIn     Radial bend factor.
     * @param rotDeg     Rotation angle (degrees).
     * @param posOff     Translation offset.
     * @param rotOff     Rotation origin.
     */
    explicit SpiralParams(std::size_t colorCount,
                          float widthIn = 1.0f,
                          float bendIn = 0.0f,
                          float rotDeg = 0.0f,
                          Vector2D posOff = {},
                          Vector2D rotOff = {}) :
    colors(colorCount, RGBColor(255, 255, 255)),
    positionOffset(posOff),
    rotationOffset(rotOff),
    width(widthIn),
    bend(bendIn),
    rotationAngle(rotDeg) {
    }

    /**
     * @brief Construct from std::vector palette.
     * @param pal        Source palette.
     * @param widthIn    Spiral frequency.
     * @param bendIn     Radial bend factor.
     * @param rotDeg     Rotation angle (degrees).
     * @param posOff     Translation offset.
     * @param rotOff     Rotation origin.
     */
    explicit SpiralParams(const std::vector<RGBColor>& pal,
                          float widthIn = 1.0f,
                          float bendIn = 0.0f,
                          float rotDeg = 0.0f,
                          Vector2D posOff = {},
                          Vector2D rotOff = {}) :
    colors(pal),
    positionOffset(posOff),
    rotationOffset(rotOff),
    width(widthIn),
    bend(bendIn),
    rotationAngle(rotDeg) {
    }

    /**
     * @brief Resize the color array.
     * @param count Number of colors to allocate (initialized to white).
     */
    void ResizeColors(std::size_t count) {
        colors.resize(count, RGBColor(255, 255, 255));
    }

    /**
     * @brief Get the number of colors in the palette.
     * @return Number of colors.
     */
    std::size_t ColorCount() const {
        return colors.size();
    }

    /**
     * @brief Get mutable pointer to color array.
     * @return Pointer to color data.
     */
    RGBColor* ColorData() {
        return colors.data();
    }

    /**
     * @brief Get const pointer to color array.
     * @return Pointer to color data.
     */
    const RGBColor* ColorData() const {
        return colors.data();
    }

    PTX_BEGIN_FIELDS(SpiralParams)
        PTX_FIELD(SpiralParams, colors, "Colors"),
        PTX_FIELD(SpiralParams, positionOffset, "Position offset"),
        PTX_FIELD(SpiralParams, rotationOffset, "Rotation offset"),
        PTX_FIELD(SpiralParams, width, "Width"),
        PTX_FIELD(SpiralParams, bend, "Bend"),
        PTX_FIELD(SpiralParams, rotationAngle, "Rotation angle")
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(SpiralParams)
        PTX_METHOD_AUTO(SpiralParams, ResizeColors, "Resize colors"),
        PTX_METHOD_AUTO(SpiralParams, ColorCount, "Color count"),
        PTX_METHOD_OVLD(SpiralParams, ColorData, RGBColor*, "Color data"),
        PTX_METHOD_OVLD(SpiralParams, ColorData, const RGBColor*, "Color data const")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(SpiralParams)
        PTX_CTOR0(SpiralParams),
        PTX_CTOR(SpiralParams, std::size_t, float, float, float, Vector2D, Vector2D)
    PTX_END_DESCRIBE(SpiralParams)

};
