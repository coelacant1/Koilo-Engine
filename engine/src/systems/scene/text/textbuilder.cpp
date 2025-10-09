#include <ptx/systems/scene/text/textbuilder.h>

#include <algorithm>
#include <cmath>

namespace {
constexpr float kCellMargin = 10.0f;
constexpr float kHueInvertDegrees = 180.0f;

inline Vector3D MakeOffset(float x, float y) {
    return Vector3D(x, y, 0.0f);
}
}

TextBuilder::TextBuilder(std::size_t lineCount,
                         std::size_t characterWidth,
                         bool isEfficient)
    : isEfficient_(isEfficient) {
    ResizeBuffers(lineCount, characterWidth);
    ClearText();
}

TextBuilder::TextBuilder(std::size_t lineCount,
                         std::size_t characterWidth,
                         Vector2D size,
                         Vector2D position,
                         uint32_t blinkTimeMs,
                         bool isEfficient)
    : isEfficient_(isEfficient) {
    size_ = size;
    positionOffset_ = position;
    blinkPeriodMs_ = blinkTimeMs;
    ResizeBuffers(lineCount, characterWidth);
    ClearText();
}

void TextBuilder::SetMaterial(const IMaterial* material) {
    fillMaterial_ = material;
}

void TextBuilder::SetSize(Vector2D size) {
    size_ = size;
}

void TextBuilder::SetPositionOffset(Vector2D positionOffset) {
    positionOffset_ = positionOffset;
}

void TextBuilder::SetRotationOffset(Vector2D rotationOffset) {
    rotationOffset_ = rotationOffset;
}

void TextBuilder::SetRotationAngle(float rotationAngleDeg) {
    rotationAngleDeg_ = rotationAngleDeg;
}

void TextBuilder::SetBlinkTime(uint32_t blinkTimeMs) {
    blinkPeriodMs_ = blinkTimeMs;
}

void TextBuilder::SetForegroundColor(const RGBColor& color) {
    foregroundColor_.SetColor(color.R, color.G, color.B);
}

void TextBuilder::SetBackgroundColor(const RGBColor& color) {
    backgroundColor_.SetColor(color.R, color.G, color.B);
}

void TextBuilder::SetText(std::size_t line, const ptx::UString& value, bool centerText) {
    if (line >= lineCount_ || characterWidth_ == 0) {
        return;
    }

    const char* text = value.CStr();
    if (!text) {
        return;
    }

    std::size_t length = static_cast<std::size_t>(value.Length());
    if (length == 0) {
        std::fill(lines_[line].begin(), lines_[line].end(), ' ');
        return;
    }

    if (length > characterWidth_) {
        if (centerText) {
            return; // cannot centre text wider than the buffer
        }
        length = characterWidth_;
    }

    int start = 0;
    if (centerText) {
        int spacing = static_cast<int>(characterWidth_) - static_cast<int>(length);
        if (spacing < 0) {
            return;
        }
        start = spacing / 2;
    }

    std::string& lineBuffer = lines_[line];
    std::fill(lineBuffer.begin(), lineBuffer.end(), ' ');

    for (std::size_t i = 0; i < length; ++i) {
        const std::size_t index = static_cast<std::size_t>(start) + i;
        if (index < lineBuffer.size()) {
            lineBuffer[index] = text[i];
        }
    }
}

void TextBuilder::ClearText() {
    for (std::string& line : lines_) {
        std::fill(line.begin(), line.end(), ' ');
    }
}

RGBColor TextBuilder::GetRGB(const Vector3D& position,
                             const Vector3D& normal,
                             const Vector3D& uvw) const {
    if (lineCount_ == 0 || characterWidth_ == 0) {
        return backgroundColor_;
    }

    if (size_.X <= 0.0f || size_.Y <= 0.0f) {
        return backgroundColor_;
    }

    Vector3D positionL = position;

    if (!Mathematics::IsClose(rotationAngleDeg_, 0.0f, Mathematics::EPSILON)) {
        if (!isEfficient_) {
            positionL = positionL - MakeOffset(rotationOffset_.X, rotationOffset_.Y);
            Quaternion rotation = Rotation(EulerAngles(Vector3D(0.0f, 0.0f, rotationAngleDeg_),
                                                       EulerConstants::EulerOrderXYZS)).GetQuaternion();
            positionL = rotation.RotateVector(positionL);
            positionL = positionL + MakeOffset(rotationOffset_.X, rotationOffset_.Y);
        } else {
            const Vector2D tempPos(positionL.X, positionL.Y);
            const int angle = static_cast<int>(rotationAngleDeg_) % 360;
            if (Mathematics::IsClose(static_cast<float>(angle), 90.0f, 45.0f)) {
                positionL.X = tempPos.Y;
                positionL.Y = -tempPos.X;
            } else if (Mathematics::IsClose(static_cast<float>(angle), 180.0f, 45.0f)) {
                positionL.X = -tempPos.X;
                positionL.Y = -tempPos.Y;
            } else if (Mathematics::IsClose(static_cast<float>(angle), 270.0f, 45.0f)) {
                positionL.X = -tempPos.Y;
                positionL.Y = tempPos.X;
            }
        }
    }

    positionL = positionL - MakeOffset(positionOffset_.X, positionOffset_.Y);

    const float widthScale  = static_cast<float>(characterWidth_) * kCellMargin;
    const float heightScale = static_cast<float>(lineCount_) * kCellMargin;

    int x = static_cast<int>(std::floor(Mathematics::Map(positionL.X, 0.0f, size_.X, widthScale, 0.0f)));
    int y = static_cast<int>(std::floor(Mathematics::Map(positionL.Y, 0.0f, size_.Y, heightScale, 0.0f)));

    if (x < 0 || x >= static_cast<int>(widthScale) || y < 0 || y >= static_cast<int>(heightScale)) {
        return backgroundColor_;
    }

    const uint8_t charXBit = static_cast<uint8_t>(9 - (x % 10));
    const uint8_t charYBit = static_cast<uint8_t>(y % 10);

    const std::size_t rowIndex = static_cast<std::size_t>(y / 10);
    const std::size_t colIndex = static_cast<std::size_t>(x / 10);

    if (rowIndex >= lines_.size() || colIndex >= lines_[rowIndex].size()) {
        return backgroundColor_;
    }

    const char glyph = lines_[rowIndex][colIndex];
    const bool blink = BlinkOn() && glyph > 90;

    const RGBColor fillColor = SampleFill(positionL, normal, uvw);

    if (charYBit == 0 || charYBit == 9 || charXBit == 0 || charXBit == 9) {
        if (blink) {
            return RGBColor(fillColor).HueShift(kHueInvertDegrees);
        }
        return backgroundColor_;
    }

    const uint8_t rowByte = Characters::GetCharacter(glyph)[charYBit - 1];
    const bool pixelOn = ((rowByte >> (charXBit - 1)) & 0x01u) != 0;

    if (blink) {
        return pixelOn ? backgroundColor_ : RGBColor(fillColor).HueShift(kHueInvertDegrees);
    }

    if (glyph > 90) {
        return pixelOn ? RGBColor(fillColor).HueShift(kHueInvertDegrees) : backgroundColor_;
    }

    return pixelOn ? fillColor : backgroundColor_;
}

void TextBuilder::ResizeBuffers(std::size_t lineCount, std::size_t characterWidth) {
    lineCount_ = std::max<std::size_t>(1, lineCount);
    characterWidth_ = std::max<std::size_t>(1, characterWidth);
    lines_.assign(lineCount_, std::string(characterWidth_, ' '));
}

bool TextBuilder::BlinkOn() const {
    if (blinkPeriodMs_ == 0) {
        return false;
    }
    const uint32_t period = blinkPeriodMs_ * 2u;
    if (period == 0) {
        return false;
    }
    const uint32_t now = ptx::Time::Millis();
    return (now % period) >= blinkPeriodMs_;
}

RGBColor TextBuilder::SampleFill(const Vector3D& position,
                                 const Vector3D& normal,
                                 const Vector3D& uvw) const {
    if (!fillMaterial_) {
        return foregroundColor_;
    }

    const IShader* shader = fillMaterial_->GetShader();
    if (!shader) {
        return foregroundColor_;
    }

    SurfaceProperties surface(position, normal, uvw);
    return shader->Shade(surface, *fillMaterial_);
}
