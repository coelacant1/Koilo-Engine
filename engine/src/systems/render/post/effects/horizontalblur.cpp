#include <ptx/systems/render/post/effects/horizontalblur.hpp>

HorizontalBlur::HorizontalBlur(uint8_t pixels)
: pixels_(pixels) {}

void HorizontalBlur::Apply(IPixelGroup* pg) {
    if (!pg) return;

    RGBColor* src = pg->GetColors();
    RGBColor* buf = pg->GetColorBuffer();
    const uint16_t n = pg->GetPixelCount();
    if (!src || !buf || n == 0) return;

    // Map 0..1 ratio -> usable radius (at least 1, up to pixels_/2)
    const int maxRadius = (pixels_ > 1) ? (pixels_ / 2) : 1;
    const int radius = (int)Mathematics::Max(1.0f, Mathematics::Map(ratio, 0.0f, 1.0f, 1.0f, (float)maxRadius));

    for (uint16_t i = 0; i < n; ++i) {
        // start with center sample
        uint32_t accR = src[i].R;
        uint32_t accG = src[i].G;
        uint32_t accB = src[i].B;
        uint32_t count = 1;

        // walk left/right up to radius steps using adjacency indices
        uint16_t leftIdx  = i;
        uint16_t rightIdx = i;

        for (int j = 0; j < radius; ++j) {
            uint16_t tLeft  = 0;
            uint16_t tRight = 0;
            bool hasL = pg->GetLeftIndex(leftIdx,  &tLeft);
            bool hasR = pg->GetRightIndex(rightIdx, &tRight);

            if (hasL) {
                leftIdx = tLeft;
                accR += src[leftIdx].R;
                accG += src[leftIdx].G;
                accB += src[leftIdx].B;
                ++count;
            }
            if (hasR) {
                rightIdx = tRight;
                accR += src[rightIdx].R;
                accG += src[rightIdx].G;
                accB += src[rightIdx].B;
                ++count;
            }

            // if neither side progressed, we’re at an edge-stop early
            if (!hasL && !hasR) break;
        }

        const uint8_t outR = (uint8_t)Mathematics::Constrain((unsigned int)(accR / count), 0u, 255u);
        const uint8_t outG = (uint8_t)Mathematics::Constrain((unsigned int)(accG / count), 0u, 255u);
        const uint8_t outB = (uint8_t)Mathematics::Constrain((unsigned int)(accB / count), 0u, 255u);

        buf[i].R = outR;
        buf[i].G = outG;
        buf[i].B = outB;
    }

    // commit buffer back to source
    for (uint16_t i = 0; i < n; ++i) {
        src[i] = buf[i];
    }
}
