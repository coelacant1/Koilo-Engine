#include <ptx/core/color/rgbcolor.hpp>

RGBColor::RGBColor() {}

RGBColor::RGBColor(const uint8_t& R, const uint8_t& G, const uint8_t& B) {
    this->R = R;
    this->G = G;
    this->B = B;
}

RGBColor::RGBColor(const RGBColor& rgbColor) {
    this->R = rgbColor.R;
    this->G = rgbColor.G;
    this->B = rgbColor.B;
}

RGBColor::RGBColor(const Vector3D& color) {
    this->R = color.X;
    this->G = color.Y;
    this->B = color.Z;
}

void RGBColor::SetColor(const uint8_t& R, const uint8_t& G, const uint8_t& B) {
    this->R = R;
    this->G = G;
    this->B = B;
}

RGBColor RGBColor::Scale(const uint8_t& maxBrightness) {
    const int scaledR = Mathematics::Constrain(
        static_cast<int>(R) * static_cast<int>(maxBrightness) / 255,
        0,
        255);
    const int scaledG = Mathematics::Constrain(
        static_cast<int>(G) * static_cast<int>(maxBrightness) / 255,
        0,
        255);
    const int scaledB = Mathematics::Constrain(
        static_cast<int>(B) * static_cast<int>(maxBrightness) / 255,
        0,
        255);

    return RGBColor(
        static_cast<uint8_t>(scaledR),
        static_cast<uint8_t>(scaledG),
        static_cast<uint8_t>(scaledB));
}

RGBColor RGBColor::Add(const uint8_t& value) {
    int sR, sG, sB;

    sR = (uint8_t)R + (uint8_t)value;
    sG = (uint8_t)G + (uint8_t)value;
    sB = (uint8_t)B + (uint8_t)value;

    sR = sR > 255 ? 255 : sR;
    sG = sG > 255 ? 255 : sG;
    sB = sB > 255 ? 255 : sB;

    sR = sR < 0 ? 0 : sR;
    sG = sG < 0 ? 0 : sG;
    sB = sB < 0 ? 0 : sB;

    return RGBColor(sR, sG, sB);
}

RGBColor RGBColor::HueShift(const float& hueDeg) {
    //hueDeg = (int)hueDeg % 360;
    //shift color space by rotating rgb vector about diagonal vector (1, 1, 1)
    float hueRad = hueDeg * Mathematics::MPI / 180.0f;
    float hueRat = 0.5f * sinf(hueRad / 2.0f);//2.0f for quaternion creation
    Vector3D rgbVec = Vector3D(R, G, B);
    Quaternion q = Quaternion(cosf(hueRad / 2.0f), hueRat, hueRat, hueRat);

    rgbVec = q.RotateVector(rgbVec).Constrain(0.0f, 255.0f);
    
    return RGBColor(rgbVec.X, rgbVec.Y, rgbVec.Z);
}


RGBColor RGBColor::InterpolateColors(const RGBColor& a, const RGBColor& b, const float& ratio) {
    RGBColor c;

    c.R = a.R * (1.0f - ratio) + b.R * ratio;
    c.G = a.G * (1.0f - ratio) + b.G * ratio;
    c.B = a.B * (1.0f - ratio) + b.B * ratio;

    return c;
}

ptx::UString RGBColor::ToString() {
    ptx::UString r = Mathematics::DoubleToCleanString(this->R);
    ptx::UString g = Mathematics::DoubleToCleanString(this->G);
    ptx::UString b = Mathematics::DoubleToCleanString(this->B);

    return "[" + r + ", " + g + ", " + b + "]";
}

RGBColor RGBColor::operator+(const RGBColor& other) const {
    // Add components and clamp the result to the max value of 255.
    int newR = static_cast<int>(R) + other.R;
    int newG = static_cast<int>(G) + other.G;
    int newB = static_cast<int>(B) + other.B;

    return RGBColor(
        static_cast<uint8_t>(Mathematics::Constrain(newR, 0, 255)),
        static_cast<uint8_t>(Mathematics::Constrain(newG, 0, 255)),
        static_cast<uint8_t>(Mathematics::Constrain(newB, 0, 255))
    );
}

RGBColor& RGBColor::operator+=(const RGBColor& other) {
    *this = *this + other;
    return *this;
}

// --- Subtraction ---

RGBColor RGBColor::operator-(const RGBColor& other) const {
    // Subtract components and clamp the result to the min value of 0.
    int newR = static_cast<int>(R) - other.R;
    int newG = static_cast<int>(G) - other.G;
    int newB = static_cast<int>(B) - other.B;

    return RGBColor(
        static_cast<uint8_t>(Mathematics::Constrain(newR, 0, 255)),
        static_cast<uint8_t>(Mathematics::Constrain(newG, 0, 255)),
        static_cast<uint8_t>(Mathematics::Constrain(newB, 0, 255))
    );
}

RGBColor& RGBColor::operator-=(const RGBColor& other) {
    *this = *this - other;
    return *this;
}

RGBColor RGBColor::operator*(const float& scalar) const {
    // Scale each component by the scalar and clamp to the 0-255 range.
    uint8_t newR = static_cast<uint8_t>(Mathematics::Constrain(static_cast<float>(R) * scalar, 0.0f, 255.0f));
    uint8_t newG = static_cast<uint8_t>(Mathematics::Constrain(static_cast<float>(G) * scalar, 0.0f, 255.0f));
    uint8_t newB = static_cast<uint8_t>(Mathematics::Constrain(static_cast<float>(B) * scalar, 0.0f, 255.0f));
    
    return RGBColor(newR, newG, newB);
}

RGBColor RGBColor::operator*(const RGBColor& other) const {
    // Modulate by normalizing components to [0,1], multiplying, then scaling back.
    // (r1/255 * r2/255) * 255 simplifies to (r1 * r2) / 255.
    int newR = (static_cast<int>(R) * other.R) / 255;
    int newG = (static_cast<int>(G) * other.G) / 255;
    int newB = (static_cast<int>(B) * other.B) / 255;

    // Clamp results to ensure they are valid uint8_t values.
    return RGBColor(
        static_cast<uint8_t>(Mathematics::Constrain(newR, 0, 255)),
        static_cast<uint8_t>(Mathematics::Constrain(newG, 0, 255)),
        static_cast<uint8_t>(Mathematics::Constrain(newB, 0, 255))
    );
}

RGBColor& RGBColor::operator*=(const float& scalar) {
    *this = *this * scalar;
    return *this;
}

RGBColor& RGBColor::operator*=(const RGBColor& other) {
    *this = *this * other;
    return *this;
}

RGBColor RGBColor::operator/(const float& scalar) const {
    // Prevent division by zero.
    if (std::abs(scalar) < std::numeric_limits<float>::epsilon()) {
        return *this; // Or return solid white: RGBColor(255, 255, 255)
    }

    return *this * (1.0f / scalar);
}

RGBColor RGBColor::operator/(const RGBColor& other) const {
    // To "un-modulate" a color, perform the inverse of the modulation logic.
    // Check each component for division by zero.
    int newR = (other.R == 0) ? 255 : (static_cast<int>(this->R) * 255) / other.R;
    int newG = (other.G == 0) ? 255 : (static_cast<int>(this->G) * 255) / other.G;
    int newB = (other.B == 0) ? 255 : (static_cast<int>(this->B) * 255) / other.B;

    return RGBColor(
        static_cast<uint8_t>(Mathematics::Constrain(newR, 0, 255)),
        static_cast<uint8_t>(Mathematics::Constrain(newG, 0, 255)),
        static_cast<uint8_t>(Mathematics::Constrain(newB, 0, 255))
    );
}

RGBColor& RGBColor::operator/=(const float& scalar) {
    *this = *this / scalar;
    return *this;
}

RGBColor& RGBColor::operator/=(const RGBColor& other) {
    *this = *this / other;
    return *this;
}

