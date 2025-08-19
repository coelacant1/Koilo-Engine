#include "magnet.hpp"

Magnet::Magnet(float amplitude)
: amplitude_(amplitude) {}

void Magnet::SetPosition(Vector2D offset) {
    offset_ = offset;
}

void Magnet::SetAmplitude(float amplitude) {
    amplitude_ = amplitude;
}

void Magnet::Apply(IPixelGroup* pg) {
    if (!pg) return;

    RGBColor* src = pg->GetColors();
    RGBColor* buf = pg->GetColorBuffer();
    const uint16_t n = pg->GetPixelCount();
    if (!src || !buf || n == 0) return;

    const Vector2D mid = pg->GetCenterCoordinate();

    // animate center & warp strength with ratio scaling
    const float warp = fGenWarp_.Update() * Mathematics::Constrain(ratio, 0.0f, 1.0f);
    const Vector2D animOffset { fGenX_.Update() * ratio, fGenY_.Update() * ratio };

    // avoid /0
    constexpr float kEps = 1e-3f;

    for (uint16_t i = 0; i < n; ++i) {
        const Vector2D pos = pg->GetCoordinate(i) + offset_ + animOffset;
        const Vector2D dif = pos - (mid /* + optional bias if desired */);

        const float dist = Mathematics::Max(dif.Magnitude(), kEps);
        const float theta = std::atan2(dif.Y, dif.X);

        // inverse-distance pull; scaled by amplitude & animated warp
        const float pull = (amplitude_ * warp) / dist;

        // convert to integer pixel offsets (screen-space)
        const int offX = static_cast<int>(pull * std::cos(theta));
        const int offY = static_cast<int>(pull * std::sin(theta));

        uint16_t tIndex = 0;
        if (pg->GetOffsetXYIndex(i, &tIndex, offX, offY)) {
            buf[i] = src[tIndex];
        } else {
            buf[i] = RGBColor{0, 0, 0};
        }
    }

    // commit
    for (uint16_t i = 0; i < n; ++i) {
        src[i] = buf[i];
    }
}
