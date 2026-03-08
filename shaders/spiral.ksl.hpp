// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "koilo/ksl/ksl.hpp"

struct KSL_SpiralShader : public ksl::Shader {
    int colorCount = 1;
    ksl::vec3 colors[32];
    ksl::vec2 positionOffset;
    ksl::vec2 rotationOffset;
    float width = 1.0f;
    float bend = 0.0f;
    float rotationAngle = 0.0f;

    KSL_PARAMS_BEGIN(KSL_SpiralShader)
        KSL_PARAM(int,       colorCount,    "Palette size",       1, 32)
        KSL_PARAM_ARRAY(ksl::vec3, colors,  32, "Color palette")
        KSL_PARAM(float,     width,         "Spiral frequency",   0.0f, 100.0f)
        KSL_PARAM(float,     bend,          "Radial bend",        0.0f, 10.0f)
        KSL_PARAM(float,     rotationAngle, "Rotation degrees",  -360.0f, 360.0f)
        KSL_PARAM(ksl::vec2, positionOffset,"Position offset",   -1000.0f, 1000.0f)
        KSL_PARAM(ksl::vec2, rotationOffset,"Rotation pivot",    -1000.0f, 1000.0f)
    KSL_PARAMS_END

    ksl::vec4 shade(const ksl::ShadeInput& in) const override {
        if (colorCount <= 0) return ksl::vec4(0.0f);

        ksl::vec2 pos = in.position.xy();

        // Rotation around rotationOffset
        pos = pos - rotationOffset;
        if (rotationAngle != 0.0f) {
            float rad = rotationAngle * 3.14159265f / 180.0f;
            float c = ksl::cos(rad);
            float s = ksl::sin(rad);
            pos = ksl::vec2(pos.x * c - pos.y * s, pos.x * s + pos.y * c);
        }
        pos = pos + rotationOffset;
        pos = pos - positionOffset;

        float radius = ksl::length(pos);
        float angle = ksl::atan(pos.y, pos.x);

        float termA = (width != 0.0f) ? (width * angle / 3.14159265f) : 0.0f;
        float termB = (bend != 0.0f) ? (bend * ksl::pow(radius, 0.3f)) : 0.0f;

        float ratio = ksl::fract(termA + termB);
        int idx = ksl::clamp(int(ksl::floor(ratio * float(colorCount))), 0, colorCount - 1);
        return ksl::vec4(colors[idx], 1.0f);
    }
};
