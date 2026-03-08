// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "koilo/ksl/ksl.hpp"

struct KSL_ProceduralNoiseShader : public ksl::Shader {
    int colorCount = 6;
    ksl::vec3 colors[32];
    ksl::vec3 noiseScale;
    float simplexDepth = 0.0f;
    float gradientPeriod = 1.0f;

    KSL_PARAMS_BEGIN(KSL_ProceduralNoiseShader)
        KSL_PARAM(int,       colorCount,    "Spectrum size",      1, 32)
        KSL_PARAM_ARRAY(ksl::vec3, colors,  32, "Spectrum colors")
        KSL_PARAM(ksl::vec3, noiseScale,    "Noise XYZ scale",    0.0f, 100.0f)
        KSL_PARAM(float,     simplexDepth,  "Z slice / time",    -1000.0f, 1000.0f)
        KSL_PARAM(float,     gradientPeriod,"Gradient period",    0.001f, 100.0f)
    KSL_PARAMS_END

    // Simplex noise helpers (ported from Ashima Arts webgl-noise)
    ksl::vec3 mod289(ksl::vec3 x) const {
        return x - ksl::floor(x * (1.0f / 289.0f)) * 289.0f;
    }

    ksl::vec4 mod289v4(ksl::vec4 x) const {
        return x - ksl::floor(x * (1.0f / 289.0f)) * 289.0f;
    }

    ksl::vec4 permute(ksl::vec4 x) const {
        return mod289v4((x * 34.0f + ksl::vec4(1.0f)) * x);
    }

    ksl::vec4 taylorInvSqrt(ksl::vec4 r) const {
        return ksl::vec4(1.79284291400159f) - r * 0.85373472095314f;
    }

    float snoise(ksl::vec3 v) const {
        ksl::vec3 i = ksl::floor(v + ksl::vec3(ksl::dot(v, ksl::vec3(1.0f / 3.0f))));
        ksl::vec3 x0 = v - i + ksl::vec3(ksl::dot(i, ksl::vec3(1.0f / 6.0f)));

        ksl::vec3 g = ksl::step(ksl::vec3(x0.y, x0.z, x0.x), x0);
        ksl::vec3 l = ksl::vec3(1.0f) - g;
        ksl::vec3 i1 = ksl::vec3(ksl::min(g.x, l.z), ksl::min(g.y, l.x), ksl::min(g.z, l.y));
        ksl::vec3 i2 = ksl::vec3(ksl::max(g.x, l.z), ksl::max(g.y, l.x), ksl::max(g.z, l.y));

        ksl::vec3 x1 = x0 - i1 + ksl::vec3(1.0f / 6.0f);
        ksl::vec3 x2 = x0 - i2 + ksl::vec3(1.0f / 3.0f);
        ksl::vec3 x3 = x0 - ksl::vec3(0.5f);

        i = mod289(i);
        ksl::vec4 p = permute(permute(permute(
            ksl::vec4(i.z, i.z + i1.z, i.z + i2.z, i.z + 1.0f))
            + ksl::vec4(i.y, i.y + i1.y, i.y + i2.y, i.y + 1.0f))
            + ksl::vec4(i.x, i.x + i1.x, i.x + i2.x, i.x + 1.0f));

        // Gradients: 7×7 points over a square, mapped onto an octahedron
        // ns = n_ * D.wyz - D.xzx  where D = (0, 0.5, 1, 2), n_ = 1/7
        const float ns_x = 2.0f / 7.0f;
        const float ns_y = -13.0f / 14.0f;
        const float ns_z = 1.0f / 7.0f;

        ksl::vec4 j = p - ksl::vec4(49.0f) * ksl::floor(p * ns_z * ns_z);
        ksl::vec4 x_ = ksl::floor(j * ns_z);
        ksl::vec4 y_ = ksl::floor(j - ksl::vec4(7.0f) * x_);
        ksl::vec4 gx = x_ * ns_x + ksl::vec4(ns_y);
        ksl::vec4 gy = y_ * ns_x + ksl::vec4(ns_y);
        ksl::vec4 gz = ksl::vec4(1.0f) - ksl::abs(gx) - ksl::abs(gy);
        ksl::vec4 sz = ksl::step(ksl::vec4(0.0f), gz);

        gx = gx - sz * (ksl::step(ksl::vec4(0.0f), gx) - ksl::vec4(0.5f));
        gy = gy - sz * (ksl::step(ksl::vec4(0.0f), gy) - ksl::vec4(0.5f));

        ksl::vec3 g0 = ksl::vec3(gx.x, gy.x, gz.x);
        ksl::vec3 g1 = ksl::vec3(gx.y, gy.y, gz.y);
        ksl::vec3 g2 = ksl::vec3(gx.z, gy.z, gz.z);
        ksl::vec3 g3 = ksl::vec3(gx.w, gy.w, gz.w);

        ksl::vec4 norm = taylorInvSqrt(ksl::vec4(ksl::dot(g0, g0), ksl::dot(g1, g1),
                                                  ksl::dot(g2, g2), ksl::dot(g3, g3)));
        g0 = g0 * norm.x; g1 = g1 * norm.y; g2 = g2 * norm.z; g3 = g3 * norm.w;

        ksl::vec4 m = ksl::vec4(ksl::max(0.6f - ksl::dot(x0, x0), 0.0f),
                    ksl::max(0.6f - ksl::dot(x1, x1), 0.0f),
                    ksl::max(0.6f - ksl::dot(x2, x2), 0.0f),
                    ksl::max(0.6f - ksl::dot(x3, x3), 0.0f));
        m = m * m;
        m = m * m;

        return 42.0f * ksl::dot(m, ksl::vec4(ksl::dot(x0, g0), ksl::dot(x1, g1),
                                              ksl::dot(x2, g2), ksl::dot(x3, g3)));
    }

    ksl::vec4 shade(const ksl::ShadeInput& in) const override {
        if (colorCount <= 0) return ksl::vec4(0.0f);

        ksl::vec3 p = ksl::vec3(in.position.x * noiseScale.x,
                    in.position.y * noiseScale.y,
                    simplexDepth * noiseScale.z);

        float n = snoise(p);
        n = (n + 1.0f) * 0.5f; // [-1,1] -> [0,1]

        float mapped = ksl::mod(n * gradientPeriod, 1.0f) * float(colorCount);
        int i0 = ksl::clamp(int(ksl::floor(mapped)), 0, colorCount - 1);
        int i1 = (i0 + 1) % colorCount;
        float mu = mapped - ksl::floor(mapped);

        ksl::vec3 c = ksl::mix(colors[i0], colors[i1], mu);
        return ksl::vec4(c, 1.0f);
    }
};
