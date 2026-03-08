// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "koilo/ksl/ksl.hpp"

struct KSL_UVMapShader : public ksl::Shader {
    float hueAngle = 0.0f;
    bool flipU = false;
    bool flipV = false;

    KSL_PARAMS_BEGIN(KSL_UVMapShader)
        KSL_PARAM(float, hueAngle, "Hue shift degrees", -360.0f, 360.0f)
        KSL_PARAM(bool,  flipU,    "Flip U axis",       0, 1)
        KSL_PARAM(bool,  flipV,    "Flip V axis",       0, 1)
    KSL_PARAMS_END

    ksl::vec4 shade(const ksl::ShadeInput& in) const override {
        float u = in.uv.x;
        float v = in.uv.y;

        if (flipU) u = 1.0f - u;
        if (flipV) v = 1.0f - v;

        ksl::vec4 texel = ksl::sample(in.ctx->textures[0], ksl::vec2(u, v));
        ksl::vec3 c = ksl::hueShift(texel.xyz(), hueAngle);
        return ksl::vec4(c, texel.w);
    }
};
