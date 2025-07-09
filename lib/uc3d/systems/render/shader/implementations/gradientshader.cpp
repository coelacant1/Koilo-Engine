#include "gradientshader.hpp"

GradientShader::GradientShader(RGBColor* rgbColors, float gradientPeriod, bool isRadial, bool isStepped) {
    this->gradientPeriod = gradientPeriod;
    this->isRadial = isRadial;
    this->isStepped = isStepped;
    this->baseRGBColors = rgbColors;

    UpdateGradient(rgbColors);
}

void GradientShader::UpdateGradient(RGBColor* rgbColors) {
    for (uint8_t i = 0; i < colorCount; i++) {
        this->rgbColors[i] = rgbColors[i];
    }
}

void GradientShader::SetPositionOffset(Vector2D positionOffset) {
    this->positionOffset = positionOffset;
}

void GradientShader::SetRotationOffset(Vector2D rotationOffset) {
    this->rotationOffset = rotationOffset;
}

void GradientShader::SetRotationAngle(float rotationAngle) {
    this->rotationAngle = rotationAngle;
}

void GradientShader::SetGradientPeriod(float gradientPeriod) {
    this->gradientPeriod = gradientPeriod;
}

void GradientShader::GradientShift(float ratio) {
    this->gradientShift = ratio;
}

void GradientShader::HueShift(float hueDeg) {
    for (uint8_t i = 0; i < colorCount; i++) {
        rgbColors[i] = baseRGBColors[i].HueShift(hueDeg);
    }
}

void GradientShader::UpdateRGB() {
    for (uint8_t i = 0; i < colorCount; i++) {
        rgbColors[i] = baseRGBColors[i];
    }
}

RGBColor GradientShader::GetRGB(const Vector3D& position, const Vector3D& normal, const Vector3D& uvw) {
    
}
