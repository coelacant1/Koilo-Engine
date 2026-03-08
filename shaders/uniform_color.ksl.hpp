// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "koilo/ksl/ksl.hpp"

struct KSL_UniformColorShader : public ksl::Shader {
    ksl::vec3 color;

    KSL_PARAMS_BEGIN(KSL_UniformColorShader)
        KSL_PARAM(ksl::vec3, color, "Output color", 0.0f, 1.0f)
    KSL_PARAMS_END

    ksl::vec4 shade(const ksl::ShadeInput& in) const override {
        return ksl::vec4(color, 1.0f);
    }
};
