// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "koilo/ksl/ksl.hpp"

struct KSL_NormalShader : public ksl::Shader {
    KSL_NO_PARAMS

    ksl::vec4 shade(const ksl::ShadeInput& in) const override {
        ksl::vec3 n = ksl::normalize(in.normal);
        n = (n + ksl::vec3(1.0f)) * 0.5f;
        return ksl::vec4(n, 1.0f);
    }
};
