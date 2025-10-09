#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "../../../assets/font/characters.hpp"
#include "../../render/material/imaterial.hpp"
#include "../../render/shader/ishader.hpp"
#include "../../../core/color/rgbcolor.hpp"
#include "../../../core/math/mathematics.hpp"
#include "../../../core/math/rotation.hpp"
#include "../../../core/math/vector2d.hpp"
#include "../../../core/math/vector3d.hpp"
#include "../../../core/platform/time.hpp"
#include "../../../core/platform/ustring.hpp"

/**
 * @file textbuilder.h
 * @brief Glyph-based text renderer with runtime-configurable canvas dimensions.
 */

/**
 * @class TextBuilder
 * @brief Builds and renders text from an 8×8 glyph atlas with basic transform controls.
 */
class TextBuilder {
public:
    TextBuilder(std::size_t lineCount,
                std::size_t characterWidth,
                bool isEfficient = false);

    TextBuilder(std::size_t lineCount,
                std::size_t characterWidth,
                Vector2D size,
                Vector2D position,
                uint32_t blinkTimeMs,
                bool isEfficient = false);

    void SetMaterial(const IMaterial* material);
    void SetSize(Vector2D size);
    void SetPositionOffset(Vector2D positionOffset);
    void SetRotationOffset(Vector2D rotationOffset);
    void SetRotationAngle(float rotationAngleDeg);
    void SetBlinkTime(uint32_t blinkTimeMs);
    void SetForegroundColor(const RGBColor& color);
    void SetBackgroundColor(const RGBColor& color);

    void SetText(std::size_t line, const ptx::UString& value, bool centerText = false);
    void ClearText();

    std::size_t GetLineCount() const noexcept { return lineCount_; }
    std::size_t GetCharacterWidth() const noexcept { return characterWidth_; }
    bool IsEfficient() const noexcept { return isEfficient_; }
    Vector2D GetSize() const noexcept { return size_; }
    Vector2D GetPositionOffset() const noexcept { return positionOffset_; }
    Vector2D GetRotationOffset() const noexcept { return rotationOffset_; }
    float GetRotationAngle() const noexcept { return rotationAngleDeg_; }
    uint32_t GetBlinkTime() const noexcept { return blinkPeriodMs_; }

    RGBColor GetRGB(const Vector3D& position,
                    const Vector3D& normal,
                    const Vector3D& uvw) const;

private:
    static constexpr float kCellScale = 10.0f;

    const IMaterial* fillMaterial_ = nullptr;
    Vector2D size_{192.0f, 96.0f};
    Vector2D positionOffset_{0.0f, 0.0f};
    Vector2D rotationOffset_{0.0f, 0.0f};
    float rotationAngleDeg_ = 0.0f;
    uint32_t blinkPeriodMs_ = 0;
    bool isEfficient_ = false;

    RGBColor foregroundColor_{255, 255, 255};
    RGBColor backgroundColor_{0, 0, 0};

    std::size_t lineCount_ = 0;
    std::size_t characterWidth_ = 0;
    std::vector<std::string> lines_;

    void ResizeBuffers(std::size_t lineCount, std::size_t characterWidth);
    bool BlinkOn() const;
    RGBColor SampleFill(const Vector3D& position,
                        const Vector3D& normal,
                        const Vector3D& uvw) const;
};
