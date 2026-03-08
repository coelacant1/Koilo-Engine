// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "koilo/ksl/ksl.hpp"

struct KSL_PhongShader : public ksl::Shader {
    ksl::vec3 ambientColor;
    ksl::vec3 diffuseColor;
    ksl::vec3 specularColor;
    float shininess = 32.0f;

    KSL_PARAMS_BEGIN(KSL_PhongShader)
        KSL_PARAM(ksl::vec3, ambientColor,  "Ambient color",  0.0f, 1.0f)
        KSL_PARAM(ksl::vec3, diffuseColor,  "Diffuse color",  0.0f, 1.0f)
        KSL_PARAM(ksl::vec3, specularColor, "Specular color", 0.0f, 1.0f)
        KSL_PARAM(float,     shininess,     "Shininess",      1.0f, 256.0f)
    KSL_PARAMS_END

    ksl::vec4 shade(const ksl::ShadeInput& in) const override {
        ksl::vec3 N = ksl::normalize(in.normal);
        ksl::vec3 V = ksl::normalize(in.viewDir);
        ksl::vec3 result = ambientColor;

        for (int i = 0; i < in.ctx->lightCount; i++) {
            ksl::vec3 lpos = in.ctx->lights[i].position;
            ksl::vec3 lcol = in.ctx->lights[i].color;
            float lint = in.ctx->lights[i].intensity;
            float lfalloff = in.ctx->lights[i].falloff;
            float lcurve = in.ctx->lights[i].curve;

            ksl::vec3 L = lpos - in.position;
            float dist = ksl::length(L);
            L = L / ksl::max(dist, 0.0001f);

            float normDist = dist / ksl::max(lfalloff, 0.0001f);
            float atten = lint / (1.0f + 1.0f * dist + lcurve * normDist * normDist);

            float NdotL = ksl::max(ksl::dot(N, L), 0.0f);
            ksl::vec3 diff = diffuseColor * lcol * NdotL * atten;

            ksl::vec3 R = ksl::reflect(-L, N);
            float spec = ksl::pow(ksl::max(ksl::dot(V, R), 0.0f), shininess);
            ksl::vec3 specC = specularColor * lcol * spec * atten;

            result = result + diff + specC;
        }

        result = ksl::clamp(result, 0.0f, 1.0f);
        return ksl::vec4(result, 1.0f);
    }
};
