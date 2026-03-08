// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "koilo/ksl/ksl.hpp"

struct KSL_ImageSequenceShader : public ksl::Shader {
    float hueAngle = 0.0f;
    bool useUV = true;

    KSL_PARAMS_BEGIN(KSL_ImageSequenceShader)
        KSL_PARAM(float, hueAngle, "Hue shift degrees", -360.0f, 360.0f)
        KSL_PARAM(bool,  useUV,    "Use UV coordinates", 0, 1)
    KSL_PARAMS_END

    ksl::vec4 shade(const ksl::ShadeInput& in) const override {
        float u = useUV ? in.uv.x : in.position.x;
        float v = useUV ? in.uv.y : in.position.y;

        ksl::vec4 texel = ksl::sample(in.ctx->textures[0], ksl::vec2(u, v));
        ksl::vec3 c = ksl::hueShift(texel.xyz(), hueAngle);
        return ksl::vec4(c, texel.w);
    }
};
