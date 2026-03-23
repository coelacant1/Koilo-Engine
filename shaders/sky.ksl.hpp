// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "koilo/ksl/ksl.hpp"

struct KSL_SkyShader : public ksl::Shader {
    ksl::vec3 topColor;
    ksl::vec3 bottomColor;

    KSL_PARAMS_BEGIN(KSL_SkyShader)
        KSL_PARAM(ksl::vec3, topColor,    "Sky top color",    0.0f, 1.0f)
        KSL_PARAM(ksl::vec3, bottomColor, "Sky bottom color", 0.0f, 1.0f)
    KSL_PARAMS_END

    KSL_META_BEGIN(KSL_SkyShader)
        KSL_META_INT("depthTest",  0)
        KSL_META_INT("depthWrite", 0)
        KSL_META_INT("cullMode",   0)
    KSL_META_END

    ksl::vec4 shade(const ksl::ShadeInput& in) const override {
        float t = in.uv.y;
        ksl::vec3 c = ksl::mix(bottomColor, topColor, t);
        return ksl::vec4(c, 1.0f);
    }
};
