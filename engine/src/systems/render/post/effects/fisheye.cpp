#include <ptx/systems/render/post/effects/fisheye.hpp>

Fisheye::Fisheye(float amp)
: amplitude(amp) {}

void Fisheye::SetPosition(Vector2D ofs) {
    offset = ofs;
}

void Fisheye::SetAmplitude(float amp) {
    amplitude = amp;
}

void Fisheye::Apply(IPixelGroup* pg) {
    if (!pg) return;

    RGBColor* src = pg->GetColors();
    RGBColor* tmp = pg->GetColorBuffer();
    const uint16_t count = pg->GetPixelCount();
    if (!src || !tmp || count == 0) return;

    const Vector2D mid = pg->GetCenterCoordinate();
    // Animate parameters, scaled by Effect::ratio (0..1)
    const float animatedAmp = fGenWarp.Update() * ratio;
    const float halfWidth   = fGenSize.Update(); // dynamic radius
    const float ofsX        = fGenX.Update() * ratio + offset.X;
    const float ofsY        = fGenY.Update() * ratio + offset.Y;

    uint16_t tIndex = 0;

    for (uint16_t i = 0; i < count; ++i) {
        const Vector2D pos = pg->GetCoordinate(i) + Vector2D(ofsX, ofsY);
        const Vector2D dif = pos - mid;

        // r, theta in polar
        const float theta = std::atan2(dif.Y, dif.X);
        const float dist  = Vector2D::CalculateEuclideanDistance(pos, mid); // or dif.Magnitude()
        const float r     = (halfWidth > 0.0001f) ? (dist / halfWidth) : 0.0f;

        // Warp radius by exponent/amplitude
        const float expA  = (animatedAmp != 0.0f) ? animatedAmp : amplitude;
        const float newR  = std::pow(r, expA);

        // Convert back to XY offset (integer displacement in pixels)
        const int offX = (int)(newR * std::cos(theta));
        const int offY = (int)(newR * std::sin(theta));

        if (pg->GetOffsetXYIndex(i, &tIndex, offX, offY)) {
            tmp[i] = src[tIndex];
        } else {
            tmp[i].R = tmp[i].G = tmp[i].B = 0;
        }
    }

    // Copy buffer back to source
    for (uint16_t i = 0; i < count; ++i) {
        src[i] = tmp[i];
    }
}
