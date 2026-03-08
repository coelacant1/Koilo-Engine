// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "koilo/ksl/ksl.hpp"

struct KSL_DepthShader : public ksl::Shader {
    int colorCount = 2;
    ksl::vec3 colors[32];
    int axis = 2;       // 0=X, 1=Y, 2=Z
    float depth = 1.0f;
    float offset = 0.0f;

    KSL_PARAMS_BEGIN(KSL_DepthShader)
        KSL_PARAM(int,       colorCount, "Gradient colors",  1, 32)
        KSL_PARAM_ARRAY(ksl::vec3, colors, 32, "Color gradient")
        KSL_PARAM(int,       axis,       "Axis (0=X,1=Y,2=Z)", 0, 2)
        KSL_PARAM(float,     depth,      "Depth span",       0.001f, 10000.0f)
        KSL_PARAM(float,     offset,     "Axis offset",     -10000.0f, 10000.0f)
    KSL_PARAMS_END

    ksl::vec4 shade(const ksl::ShadeInput& in) const override {
        if (colorCount <= 0) return ksl::vec4(0.0f);

        float val = 0.0f;
        if (axis == 0) val = in.position.x;
        else if (axis == 1) val = in.position.y;
        else val = in.position.z;

        float halfD = depth * 0.5f;
        float t = (val - offset + halfD) / depth;
        t = ksl::clamp(t, 0.0f, 1.0f);

        float mapped = t * float(colorCount);
        int i0 = ksl::clamp(int(ksl::floor(mapped)), 0, colorCount - 1);
        int i1 = ksl::clamp(i0 + 1, 0, colorCount - 1);
        float mu = mapped - ksl::floor(mapped);

        ksl::vec3 c = ksl::mix(colors[i0], colors[i1], mu);
        return ksl::vec4(c, 1.0f);
    }
};
