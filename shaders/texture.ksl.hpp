// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "koilo/ksl/ksl.hpp"

struct KSL_TextureShader : public ksl::Shader {
    float hueAngle = 0.0f;
    float frameX = 0.0f;
    float frameY = 0.0f;
    float frameW = 1.0f;
    float frameH = 1.0f;

    KSL_PARAMS_BEGIN(KSL_TextureShader)
        KSL_PARAM(float, hueAngle, "Hue shift degrees",  -360.0f, 360.0f)
        KSL_PARAM(float, frameX,   "Frame X offset",     0.0f, 1.0f)
        KSL_PARAM(float, frameY,   "Frame Y offset",     0.0f, 1.0f)
        KSL_PARAM(float, frameW,   "Frame width",        0.0f, 1.0f)
        KSL_PARAM(float, frameH,   "Frame height",       0.0f, 1.0f)
    KSL_PARAMS_END

    ksl::vec4 shade(const ksl::ShadeInput& in) const override {
        ksl::vec2 uv = ksl::vec2(frameX + in.uv.x * frameW,
                                  frameY + in.uv.y * frameH);
        ksl::vec4 texel = ksl::sample(in.ctx->textures[0], uv);

        // Simple diffuse lighting
        ksl::vec3 N = ksl::normalize(in.normal);
        ksl::vec3 L = ksl::normalize(ksl::vec3(0.3f, 1.0f, 0.5f));
        float NdotL = ksl::max(ksl::dot(N, L), 0.2f);

        ksl::vec3 c = texel.xyz() * NdotL;
        return ksl::vec4(c, texel.w);
    }
};
