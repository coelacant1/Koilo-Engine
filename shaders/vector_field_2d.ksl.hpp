// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "koilo/ksl/ksl.hpp"

struct KSL_VectorField2DShader : public ksl::Shader {
    float sizeX = 100.0f;
    float sizeY = 100.0f;
    float positionX = 0.0f;
    float positionY = 0.0f;
    float rotation = 0.0f;

    KSL_PARAMS_BEGIN(KSL_VectorField2DShader)
        KSL_PARAM(float, sizeX,     "Field width",     0.0f, 10000.0f)
        KSL_PARAM(float, sizeY,     "Field height",    0.0f, 10000.0f)
        KSL_PARAM(float, positionX, "Field center X",  -10000.0f, 10000.0f)
        KSL_PARAM(float, positionY, "Field center Y",  -10000.0f, 10000.0f)
        KSL_PARAM(float, rotation,  "Rotation degrees", -360.0f, 360.0f)
    KSL_PARAMS_END

    ksl::vec4 shade(const ksl::ShadeInput& in) const override {
        float px = in.uv.x * sizeX;
        float py = in.uv.y * sizeY;

        ksl::vec2 coord = ksl::vec2(px - positionX + sizeX * 0.5f,
                        py - positionY + sizeY * 0.5f);

        if (ksl::abs(rotation) > 0.001f) {
            ksl::vec2 center = ksl::vec2(sizeX * 0.5f, sizeY * 0.5f);
            coord = ksl::rotate2d(coord, rotation, center);
        }

        if (coord.x < 0.0f || coord.x > sizeX ||
            coord.y < 0.0f || coord.y > sizeY)
            return ksl::vec4(0.0f, 0.0f, 0.0f, 1.0f);

        // Sample density from texture (engine packs density grid as texture)
        ksl::vec2 uv = ksl::vec2(coord.x / sizeX, coord.y / sizeY);
        ksl::vec4 texel = ksl::sample(in.ctx->textures[0], uv);
        float density = texel.x;

        // Blue to red color mapping
        return ksl::vec4(density, 0.0f, 1.0f - density, 1.0f);
    }
};
