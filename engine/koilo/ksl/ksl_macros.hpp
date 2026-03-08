// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ksl_shader.hpp"

// Reflection-style macros for KSL shader parameters.
// Usage pattern mirrors KL_BEGIN_FIELDS / KL_FIELD / KL_END_FIELDS.
//
// struct MyShader : public ksl::Shader {
//     using Self = MyShader;
//     float speed = 1.0f;
//     int colorCount = 3;
//     ksl::vec3 colors[32];
//
//     KSL_PARAMS_BEGIN(MyShader)
//         KSL_PARAM(float, speed, "Animation speed", 0.0f, 100.0f)
//         KSL_PARAM(int, colorCount, "Palette size", 1, 32)
//         KSL_PARAM_ARRAY(ksl::vec3, colors, 32, "Color palette")
//     KSL_PARAMS_END
//
//     ksl::vec4 shade(const ksl::ShadeInput& in) const override { ... }
// };

#define KSL_PARAMS_BEGIN(CLASS)                                             \
    static ksl::ParamList Params() {                                        \
        using Self = CLASS;                                                 \
        static const ksl::ParamDecl _params[] = {

#define KSL_PARAM(TYPE, NAME, DESC, MIN, MAX)                               \
    ksl::ParamDecl{ #NAME, ksl::TypeID<TYPE>(), DESC,                       \
                    offsetof(Self, NAME), float(MIN), float(MAX),           \
                    ksl::ParamFlags::None, 0 },

#define KSL_PARAM_ARRAY(TYPE, NAME, SIZE, DESC)                             \
    ksl::ParamDecl{ #NAME, ksl::TypeID<TYPE>(), DESC,                       \
                    offsetof(Self, NAME), 0, float(SIZE),                   \
                    ksl::ParamFlags::Array, SIZE },

#define KSL_PARAMS_END                                                      \
        };                                                                  \
        return ksl::ParamList{ _params,                                     \
            static_cast<int>(sizeof(_params) / sizeof(_params[0])) };       \
    }

// For shaders with no parameters
#define KSL_NO_PARAMS                                                       \
    static ksl::ParamList Params() {                                        \
        return ksl::ParamList{ nullptr, 0 };                                \
    }
