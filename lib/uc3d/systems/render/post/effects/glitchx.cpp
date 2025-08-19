#include "glitchx.hpp"

// ----- GlitchX -----
GlitchX::GlitchX(uint8_t pixels)
: pixels_(pixels) {}

// Core algorithm: for each pixel i,
//  - choose a random horizontal offset in [-blurRange, +blurRange]
//  - optionally "skip-streak" a few pixels ahead (glitch line length)
//  - sometimes swap color channels for a harsher glitch
void GlitchX::Apply(IPixelGroup* pg) {
    if (!pg) return;

    RGBColor* src = pg->GetColors();
    RGBColor* buf = pg->GetColorBuffer();
    const uint16_t n = pg->GetPixelCount();
    if (!src || !buf || n == 0) return;

    // Convert ratio (0..1) to a practical displacement window
    const int maxSpan = (pixels_ > 1) ? (pixels_ / 2) : 1;
    const int blurRange = static_cast<int>(Mathematics::Map(ratio, 0.0f, 1.0f, 1.0f, float(maxSpan)));

    // Safety clamp
    const int span = (blurRange > 0) ? blurRange : 1;

    uint16_t i = 0;
    while (i < n) {
        const int dx = uc3d::Random::Int(-span, span);
        const int streak = uc3d::Random::Int(1, span);
        const bool swapRGB = (streak < span / 2);

        uint16_t sampleIndex = 0;
        const bool valid = pg->GetOffsetXIndex(i, &sampleIndex, dx);

        RGBColor sample{};
        if (valid) {
            sample = src[sampleIndex];
            if (swapRGB) {
                // rotate channels B->R, R->G, G->B
                const uint8_t r = sample.R, g = sample.G, b = sample.B;
                sample.R = b; sample.G = r; sample.B = g;
            }
        } else {
            sample.R = sample.G = sample.B = 0;
        }

        // write the streak (without overrunning the buffer)
        uint16_t run = 0;
        for (; run < streak && (i + run) < n; ++run) {
            buf[i + run] = sample;
        }
        i += run;
    }

    // copy back
    for (uint16_t k = 0; k < n; ++k) {
        src[k] = buf[k];
    }
}
