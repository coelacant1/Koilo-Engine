// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "koilo/ksl/ksl.hpp"

struct KSL_GradientShader : public ksl::Shader {
    int colorCount = 1;
    ksl::vec3 colors[32];
    ksl::vec2 positionOffset;
    ksl::vec2 rotationOffset;
    float gradientPeriod = 1.0f;
    float rotationAngle = 0.0f;
    float gradientShift = 0.0f;
    int isRadial = 0;
    int isStepped = 0;

    KSL_PARAMS_BEGIN(KSL_GradientShader)
        KSL_PARAM(int,       colorCount,    "Palette size",       1, 32)
        KSL_PARAM_ARRAY(ksl::vec3, colors,  32, "Color palette")
        KSL_PARAM(float,     gradientPeriod,"Repeat period",      0.001f, 10000.0f)
        KSL_PARAM(float,     rotationAngle, "Rotation degrees",  -360.0f, 360.0f)
        KSL_PARAM(float,     gradientShift, "Phase shift (0-1)",  0.0f, 1.0f)
        KSL_PARAM(int,       isRadial,      "Radial mode",        0, 1)
        KSL_PARAM(int,       isStepped,     "Stepped mode",       0, 1)
        KSL_PARAM(ksl::vec2, positionOffset,"Position offset",   -1000.0f, 1000.0f)
        KSL_PARAM(ksl::vec2, rotationOffset,"Rotation pivot",    -1000.0f, 1000.0f)
    KSL_PARAMS_END

    ksl::vec4 shade(const ksl::ShadeInput& in) const override {
        if (colorCount <= 0) return ksl::vec4(0.0f);

        ksl::vec2 pos = in.position.xy();

        // Rotation
        if (rotationAngle != 0.0f) {
            pos = pos - rotationOffset;
            float rad = rotationAngle * 3.14159265f / 180.0f;
            float c = ksl::cos(rad);
            float s = ksl::sin(rad);
            pos = ksl::vec2(pos.x * c - pos.y * s, pos.x * s + pos.y * c);
            pos = pos + rotationOffset;
        }

        pos = pos - positionOffset;
        pos.x = pos.x + gradientShift * gradientPeriod;

        float phase = 0.0f;
        if (isRadial != 0) {
            phase = ksl::abs(ksl::mod(ksl::length(pos), gradientPeriod));
        } else {
            phase = ksl::abs(ksl::mod(pos.x, gradientPeriod));
        }

        float maxP = (gradientPeriod > 0.00001f) ? gradientPeriod : 1.0f;
        float mapped = phase / maxP * float(colorCount);
        float t = ksl::mod(mapped, float(colorCount));
        if (t < 0.0f) t = t + float(colorCount);

        int i0 = ksl::clamp(int(ksl::floor(t)), 0, colorCount - 1);
        int i1 = (colorCount == 1) ? 0 : ((i0 + 1) % colorCount);

        if (isStepped != 0 || colorCount == 1) return ksl::vec4(colors[i0], 1.0f);

        float mu = t - ksl::floor(t);
        ksl::vec3 c = ksl::mix(colors[i0], colors[i1], mu);
        return ksl::vec4(c, 1.0f);
    }
};
