// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "koilo/ksl/ksl.hpp"

struct KSL_TVStaticShader : public ksl::Shader {
    int noiseColorCount = 2;
    ksl::vec3 noiseColors[32];
    float noiseScaleX = 10.0f;
    float noiseScaleY = 10.0f;
    float noiseScaleZ = 10.0f;
    float noiseZ = 0.0f;
    float noiseGradientPeriod = 1.0f;
    float noiseHueDeg = 0.0f;
    int scanColorCount = 2;
    ksl::vec3 scanColors[32];
    float scanGradientPeriod = 0.1f;
    float scanShift = 0.0f;
    float scanMultiplyOpacity = 0.75f;
    float barsCenterX = 0.0f;
    float barsCenterY = 0.0f;
    float barsSizeX = 100.0f;
    float barsSizeY = 100.0f;
    float barsHueDeg = 0.0f;
    float barsSoftness = 2.0f;

    KSL_PARAMS_BEGIN(KSL_TVStaticShader)
        KSL_PARAM(int,   noiseColorCount,     "Noise gradient count",  2, 32)
        KSL_PARAM_ARRAY(ksl::vec3, noiseColors, 32, "Noise gradient colors")
        KSL_PARAM(float, noiseScaleX,         "Noise scale X",         0.01f, 100.0f)
        KSL_PARAM(float, noiseScaleY,         "Noise scale Y",         0.01f, 100.0f)
        KSL_PARAM(float, noiseScaleZ,         "Noise scale Z",         0.01f, 100.0f)
        KSL_PARAM(float, noiseZ,              "Noise Z coordinate",    -1000.0f, 1000.0f)
        KSL_PARAM(float, noiseGradientPeriod, "Noise gradient period", 0.01f, 10.0f)
        KSL_PARAM(float, noiseHueDeg,         "Noise hue shift",       -360.0f, 360.0f)
        KSL_PARAM(int,   scanColorCount,      "Scan gradient count",   2, 32)
        KSL_PARAM_ARRAY(ksl::vec3, scanColors, 32, "Scan gradient colors")
        KSL_PARAM(float, scanGradientPeriod,  "Scan gradient period",  0.01f, 10.0f)
        KSL_PARAM(float, scanShift,           "Scan shift",            -10.0f, 10.0f)
        KSL_PARAM(float, scanMultiplyOpacity, "Scan blend opacity",    0.0f, 1.0f)
        KSL_PARAM(float, barsCenterX,         "Bars center X",         -500.0f, 500.0f)
        KSL_PARAM(float, barsCenterY,         "Bars center Y",         -500.0f, 500.0f)
        KSL_PARAM(float, barsSizeX,           "Bars width",            0.0f, 1000.0f)
        KSL_PARAM(float, barsSizeY,           "Bars height",           0.0f, 1000.0f)
        KSL_PARAM(float, barsHueDeg,          "Bars hue shift",        -360.0f, 360.0f)
        KSL_PARAM(float, barsSoftness,        "Bars edge softness",    0.0f, 50.0f)
    KSL_PARAMS_END

    ksl::vec3 getNoiseGradient(float t) const {
        if (noiseColorCount <= 1) return ksl::hueShift(noiseColors[0], noiseHueDeg);
        float idx = t * float(noiseColorCount - 1);
        int i0 = int(ksl::clamp(ksl::floor(idx), 0.0f, float(noiseColorCount - 2)));
        int i1 = i0 + 1;
        float f = idx - float(i0);
        ksl::vec3 c0 = ksl::hueShift(noiseColors[i0], noiseHueDeg);
        ksl::vec3 c1 = ksl::hueShift(noiseColors[i1], noiseHueDeg);
        return ksl::mix(c0, c1, f);
    }

    ksl::vec3 getScanGradient(float t) const {
        if (scanColorCount <= 1) return scanColors[0];
        float idx = t * float(scanColorCount - 1);
        int i0 = int(ksl::clamp(ksl::floor(idx), 0.0f, float(scanColorCount - 2)));
        int i1 = i0 + 1;
        float f = idx - float(i0);
        return ksl::mix(scanColors[i0], scanColors[i1], f);
    }

    ksl::vec4 shade(const ksl::ShadeInput& in) const override {
        if (noiseColorCount < 1 || scanColorCount < 1)
            return ksl::vec4(0.0f, 0.0f, 0.0f, 1.0f);

        // 1) Base noise color
        ksl::vec3 ns = ksl::vec3(in.position.x * noiseScaleX,
                                 in.position.y * noiseScaleY,
                                 noiseZ * noiseScaleZ);
        float n = ksl::noise3d(ns);
        float s = n * 0.5f + 0.5f;
        float t = ksl::fract(s / noiseGradientPeriod);
        ksl::vec3 base = getNoiseGradient(t);

        // 2) Scanline color
        float scanU = ksl::fract(in.position.x / scanGradientPeriod + scanShift);
        ksl::vec3 scanCol = getScanGradient(scanU);

        // Multiply blend with opacity
        ksl::vec3 mul = ksl::vec3(base.x * scanCol.x, base.y * scanCol.y, base.z * scanCol.z);
        ksl::vec3 mix1 = ksl::mix(base, mul, scanMultiplyOpacity);

        // 3) Color bars
        float bx = in.position.x - barsCenterX;
        float by = in.position.y - barsCenterY;
        float halfW = barsSizeX * 0.5f;
        float halfH = barsSizeY * 0.5f;

        ksl::vec3 bars = ksl::vec3(0.0f);
        if (bx >= -halfW && bx <= halfW && by >= -halfH && by <= halfH) {
            float bu = (bx + halfW) / barsSizeX;
            int barIdx = int(ksl::clamp(ksl::floor(bu * 7.0f), 0.0f, 6.0f));

            // Canonical bar colors (gray, yellow, cyan, green, magenta, red, blue)
            ksl::vec3 barColor = ksl::vec3(0.75f);
            if (barIdx == 1) barColor = ksl::vec3(0.75f, 0.75f, 0.0f);
            if (barIdx == 2) barColor = ksl::vec3(0.0f, 0.75f, 0.75f);
            if (barIdx == 3) barColor = ksl::vec3(0.0f, 0.75f, 0.0f);
            if (barIdx == 4) barColor = ksl::vec3(0.75f, 0.0f, 0.75f);
            if (barIdx == 5) barColor = ksl::vec3(0.75f, 0.0f, 0.0f);
            if (barIdx == 6) barColor = ksl::vec3(0.0f, 0.0f, 0.75f);

            barColor = ksl::hueShift(barColor, barsHueDeg);

            // Vertical soft mask
            float bv = (by + halfH) / barsSizeY;
            float vmTop = ksl::smoothstep(0.0f, 0.05f, bv);
            float vmBot = ksl::smoothstep(0.95f, 1.0f, bv);
            float vm = ksl::clamp(vmTop - vmBot, 0.0f, 1.0f);

            // Horizontal edge softness
            float barU = bu * 7.0f - float(barIdx);
            float soft = barsSoftness / barsSizeX;
            float eLeft = ksl::smoothstep(0.0f, soft, barU);
            float eRight = 1.0f - ksl::smoothstep(1.0f - soft, 1.0f, barU);
            float edge = ksl::clamp(eLeft + eRight - 1.0f, 0.0f, 1.0f);

            bars = barColor * vm * edge;
        }

        // Lighten blend
        ksl::vec3 result = ksl::vec3(ksl::max(mix1.x, bars.x),
                      ksl::max(mix1.y, bars.y),
                      ksl::max(mix1.z, bars.z));

        return ksl::vec4(result, 1.0f);
    }
};
