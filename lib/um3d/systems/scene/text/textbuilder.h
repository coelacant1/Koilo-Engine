#pragma once

#include <stdint.h>

#include "../../assets/font/characters.h"
#include "../../render/material/material.h"
#include "../../render/shader/simple.h"
#include "../../../core/math/mathematics.hpp"
#include "../../../core/math/rotation.hpp"
#include "../../../core/math/vector2d.hpp"
#include "../../../core/math/vector3d.hpp"

template<uint8_t lineCount, uint8_t characterWidth>
class TextBuilder : public IMaterial {
private:
    SimpleMaterial baseMaterial = SimpleMaterial(RGBColor(255, 255, 255));
    IMaterial* material = &baseMaterial;
    RGBColor white = RGBColor(255, 255, 255);
    RGBColor black = RGBColor(0, 0, 0);
    Vector2D size;
    Vector2D positionOffset;
    Vector2D rotationOffset;
    float rotationAngle = 0.0f;//rotate input xyPosition
    char lines[lineCount][characterWidth];
    uint16_t blinkTime;
    bool isEfficient = false;

public:
    TextBuilder(bool isEfficient = false);

    TextBuilder(Vector2D size, Vector2D position, uint16_t blinkTime, bool isEfficient = false);

    void SetMaterial(Material* material);

    void SetSize(Vector2D size);

    void SetPositionOffset(Vector2D positionOffset);

    void SetRotationOffset(Vector2D rotationOffset);

    void SetRotationAngle(float rotationAngle);

    void SetBlinkTime(uint16_t blinkTime);

    void SetText(uint8_t line, um3d::UString value, bool centerText = false);

    void ClearText();

    RGBColor GetRGB(const Vector3D& position, const Vector3D& normal, const Vector3D& uvw) override;
};

#include "textbuilder.tpp"
