// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "koilo/ksl/ksl.hpp"

struct KSL_HorizontalRainbowShader : public ksl::Shader {
    int colorCount = 6;
    ksl::vec3 colors[32];
    ksl::vec2 positionOffset;
    float rotationDeg = 90.0f;
    float gradientPeriod = 96.0f;

    KSL_PARAMS_BEGIN(KSL_HorizontalRainbowShader)
        KSL_PARAM(int,       colorCount,    "Spectrum size",      1, 32)
        KSL_PARAM_ARRAY(ksl::vec3, colors,  32, "Spectrum colors")
        KSL_PARAM(float,     rotationDeg,   "Rotation degrees",  -360.0f, 360.0f)
        KSL_PARAM(float,     gradientPeriod,"Repeat period",      0.001f, 10000.0f)
        KSL_PARAM(ksl::vec2, positionOffset,"Scroll offset",     -1000.0f, 1000.0f)
    KSL_PARAMS_END

    ksl::vec4 shade(const ksl::ShadeInput& in) const override {
        if (colorCount <= 0) return ksl::vec4(0.0f);

        ksl::vec2 pos = in.position.xy() - positionOffset;

        // Apply rotation
        float rad = rotationDeg * 3.14159265f / 180.0f;
        float c = ksl::cos(rad);
        float s = ksl::sin(rad);
        float projected = pos.x * c + pos.y * s;

        float phase = ksl::mod(projected, gradientPeriod);
        if (phase < 0.0f) phase = phase + gradientPeriod;

        float t = phase / gradientPeriod * float(colorCount);
        int i0 = int(ksl::floor(t));
        i0 = i0 % colorCount;
        if (i0 < 0) i0 = i0 + colorCount;
        int i1 = (i0 + 1) % colorCount;
        float mu = t - ksl::floor(t);

        ksl::vec3 col = ksl::mix(colors[i0], colors[i1], mu);
        return ksl::vec4(col, 1.0f);
    }
};
