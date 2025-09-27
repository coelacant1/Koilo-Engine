#pragma once

#include <stdint.h>

#include "../../assets/font/characters.h"
#include "../../render/material/material.h"
#include "../../render/shader/simple.h"
#include "../../../core/math/mathematics.hpp"
#include "../../../core/math/rotation.hpp"
#include "../../../core/math/vector2d.hpp"
#include "../../../core/math/vector3d.hpp"

/**
 * @file textbuilder.hpp
 * @brief Glyph-based text renderer that samples characters into RGB output.
 *
 * @details
 * Renders fixed-width glyphs from the Characters font table into a virtual canvas and
 * returns colors per-fragment through @ref GetRGB. Text is organized into a compile-time
 * number of lines and characters per line. A material can be supplied to shade the glyph
 * pixels; otherwise a simple white material is used.
 *
 * @tparam lineCount      Number of text lines stored internally.
 * @tparam characterWidth Characters per line (fixed-width buffer).
 */

/**
 * @class TextBuilder
 * @brief Builds and renders text from an 8×8 glyph atlas with basic transform controls.
 *
 * Transform controls:
 * - @ref SetSize sets the logical canvas size.
 * - @ref SetPositionOffset shifts the text center.
 * - @ref SetRotationOffset sets the rotation pivot.
 * - @ref SetRotationAngle rotates input positions around the pivot (degrees).
 *
 * Text API:
 * - @ref SetText writes a line buffer (optionally centered).
 * - @ref ClearText clears all lines.
 *
 * Visuals:
 * - Colors are produced from the active @ref material; defaults to a white @ref SimpleMaterial.
 * - Blink timing can be adjusted with @ref SetBlinkTime for cursor/FX logic.
 */
template<uint8_t lineCount, uint8_t characterWidth>
class TextBuilder : public IMaterial {
private:
    SimpleMaterial baseMaterial = SimpleMaterial(RGBColor(255, 255, 255)); ///< Default fallback material.
    IMaterial* material = &baseMaterial;                                    ///< Active material (non-owning).
    RGBColor white = RGBColor(255, 255, 255);                               ///< Foreground color.
    RGBColor black = RGBColor(0, 0, 0);                                     ///< Background color.
    Vector2D size;                                                          ///< Logical canvas size.
    Vector2D positionOffset;                                                ///< Translation of text center.
    Vector2D rotationOffset;                                                ///< Rotation pivot.
    float rotationAngle = 0.0f;                                             ///< Rotation angle in degrees.
    char lines[lineCount][characterWidth];                                   ///< Text buffer [lineCount × characterWidth].
    uint16_t blinkTime;                                                     ///< Blink period (ticks/ms as defined by caller).
    bool isEfficient = false;                                               ///< Optional optimization flag.

public:
    /**
     * @brief Construct with optional efficiency flag.
     * @param isEfficient Optional optimization toggle.
     */
    TextBuilder(bool isEfficient = false);

    /**
     * @brief Construct and set size, position, and blink time.
     * @param size       Logical canvas size.
     * @param position   Initial position offset.
     * @param blinkTime  Blink timing parameter.
     * @param isEfficient Optional optimization toggle.
     */
    TextBuilder(Vector2D size, Vector2D position, uint16_t blinkTime, bool isEfficient = false);

    /**
     * @brief Set the active material used to shade glyph pixels.
     * @param material Pointer to material (non-owning).
     */
    void SetMaterial(Material* material);

    /**
     * @brief Set logical canvas size used for coordinate mapping.
     * @param size Width/height in world units.
     */
    void SetSize(Vector2D size);

    /**
     * @brief Set position offset (translation) applied before sampling.
     * @param positionOffset Center offset.
     */
    void SetPositionOffset(Vector2D positionOffset);

    /**
     * @brief Set rotation pivot.
     * @param rotationOffset Pivot point for rotation.
     */
    void SetRotationOffset(Vector2D rotationOffset);

    /**
     * @brief Set rotation angle in degrees around @ref rotationOffset.
     * @param rotationAngle Angle in degrees.
     */
    void SetRotationAngle(float rotationAngle);

    /**
     * @brief Set blink timing used for effects (e.g., cursor).
     * @param blinkTime Timing parameter (ticks/ms as defined by caller).
     */
    void SetBlinkTime(uint16_t blinkTime);

    /**
     * @brief Write text to a specific line.
     * @param line       Zero-based line index < @p lineCount.
     * @param value      Text to copy (truncated/padded to @p characterWidth).
     * @param centerText If true, horizontally center within the line buffer.
     */
    void SetText(uint8_t line, ptx::UString value, bool centerText = false);

    /**
     * @brief Clear all line buffers.
     */
    void ClearText();

    /**
     * @brief Sample RGB color at a position for the current text state.
     * @param position World/fragment position.
     * @param normal   Surface normal (unused by default).
     * @param uvw      UVW or auxiliary vector (may be used by the material).
     * @return Shaded color for the given position.
     */
    RGBColor GetRGB(const Vector3D& position, const Vector3D& normal, const Vector3D& uvw) override;
};

#include "textbuilder.tpp"
