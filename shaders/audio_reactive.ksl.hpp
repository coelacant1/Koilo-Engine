// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "koilo/ksl/ksl.hpp"

struct KSL_AudioReactiveShader : public ksl::Shader {
    int gradientCount = 2;
    ksl::vec3 gradientColors[32];
    float hueDeg = 0.0f;
    float sizeHalfX = 50.0f;
    float sizeHalfY = 50.0f;
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    float angleDeg = 0.0f;
    bool circular = false;
    float radius = 50.0f;

    KSL_PARAMS_BEGIN(KSL_AudioReactiveShader)
        KSL_PARAM(int,   gradientCount, "Gradient color count", 2, 32)
        KSL_PARAM_ARRAY(ksl::vec3, gradientColors, 32, "Gradient colors")
        KSL_PARAM(float, hueDeg,    "Hue shift degrees",  -360.0f, 360.0f)
        KSL_PARAM(float, sizeHalfX, "Half width",         0.0f, 1000.0f)
        KSL_PARAM(float, sizeHalfY, "Half height",        0.0f, 1000.0f)
        KSL_PARAM(float, offsetX,   "Offset X",           -1000.0f, 1000.0f)
        KSL_PARAM(float, offsetY,   "Offset Y",           -1000.0f, 1000.0f)
        KSL_PARAM(float, angleDeg,  "Rotation angle",     -360.0f, 360.0f)
        KSL_PARAM(bool,  circular,  "Circular mode",      0, 1)
        KSL_PARAM(float, radius,    "Ring radius",         0.0f, 500.0f)
    KSL_PARAMS_END

    ksl::vec3 getGradientColor(float t) const {
        if (gradientCount <= 1) return ksl::hueShift(gradientColors[0], hueDeg);
        float idx = t * float(gradientCount - 1);
        int i0 = int(ksl::clamp(ksl::floor(idx), 0.0f, float(gradientCount - 2)));
        int i1 = i0 + 1;
        float f = idx - float(i0);
        ksl::vec3 c0 = ksl::hueShift(gradientColors[i0], hueDeg);
        ksl::vec3 c1 = ksl::hueShift(gradientColors[i1], hueDeg);
        return ksl::mix(c0, c1, f);
    }

    ksl::vec4 shade(const ksl::ShadeInput& in) const override {
        if (gradientCount < 2 || in.ctx->sampleCount == 0)
            return ksl::vec4(0.0f, 0.0f, 0.0f, 1.0f);

        ksl::vec2 p = ksl::vec2(in.position.x, in.position.y);
        ksl::vec2 off = ksl::vec2(offsetX, offsetY);
        ksl::vec2 rPos = p;
        if (ksl::abs(angleDeg) > 0.1f) {
            rPos = ksl::rotate2d(p, angleDeg, off) - off;
        } else {
            rPos = p - off;
        }

        if (rPos.x < -sizeHalfX || rPos.x > sizeHalfX)
            return ksl::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        if (rPos.y < -sizeHalfY || rPos.y > sizeHalfY)
            return ksl::vec4(0.0f, 0.0f, 0.0f, 1.0f);

        ksl::vec2 q = rPos;
        if (circular) {
            float a = ksl::atan(rPos.y, rPos.x) / (2.0f * 3.14159f) * sizeHalfY;
            float r = ksl::length(rPos);
            q = ksl::vec2(a, r);
        }

        float sampleCountF = float(in.ctx->sampleCount);
        float fx = ksl::map(q.x, -sizeHalfX, sizeHalfX, sampleCountF, 0.0f);
        int x0 = int(ksl::clamp(ksl::floor(fx), 0.0f, float(in.ctx->sampleCount - 1)));
        int x1 = int(ksl::clamp(float(x0 + 1), 0.0f, float(in.ctx->sampleCount - 1)));
        float t = fx - float(x0);

        float s0 = in.ctx->audioSamples[x0];
        float s1 = in.ctx->audioSamples[x1];
        float height = ksl::cosineInterpolation(s0, s1, t);

        if (!circular) {
            float yColor = ksl::map(q.y, 0.0f, sizeHalfY, 1.0f, 0.0f);
            float inside = 1.0f - (height * 4.0f + 0.15f) - yColor;
            if (inside < 0.0f) {
                float g = ksl::clamp(1.0f - height - yColor, 0.0f, 1.0f);
                ksl::vec3 c = getGradientColor(g);
                return ksl::vec4(c, 1.0f);
            }
            return ksl::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        } else {
            float band = height * 150.0f;
            float r = q.y;
            float edge = radius - 5.0f;
            if ((r + band > edge) && (r - band < edge)) {
                float yColor = ksl::map(r, 0.0f, sizeHalfY, 1.0f, 0.0f);
                float g = ksl::clamp(1.0f + height - yColor, 0.0f, 1.0f);
                ksl::vec3 c = getGradientColor(g);
                return ksl::vec4(c, 1.0f);
            }
            return ksl::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        }
    }
};
