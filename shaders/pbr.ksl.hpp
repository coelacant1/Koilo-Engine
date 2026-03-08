// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "koilo/ksl/ksl.hpp"

struct KSL_PBRShader : public ksl::Shader {
    ksl::vec3 albedo;
    float roughness = 0.5f;
    float metallic = 0.0f;
    float ao = 1.0f;
    ksl::vec3 emissive;

    KSL_PARAMS_BEGIN(KSL_PBRShader)
        KSL_PARAM(ksl::vec3, albedo,    "Albedo color",  0.0f, 1.0f)
        KSL_PARAM(float,     roughness, "Roughness",     0.0f, 1.0f)
        KSL_PARAM(float,     metallic,  "Metallic",      0.0f, 1.0f)
        KSL_PARAM(float,     ao,        "Ambient occl",  0.0f, 1.0f)
        KSL_PARAM(ksl::vec3, emissive,  "Emissive color", 0.0f, 10.0f)
    KSL_PARAMS_END

    ksl::vec4 shade(const ksl::ShadeInput& in) const override {
        ksl::vec3 N = ksl::normalize(in.normal);
        ksl::vec3 V = ksl::normalize(in.viewDir);

        ksl::vec3 F0 = ksl::mix(ksl::vec3(0.04f), albedo, metallic);
        float a = roughness * roughness;
        float a2 = a * a;

        ksl::vec3 Lo = ksl::vec3(0.0f);

        for (int i = 0; i < in.ctx->lightCount; i++) {
            ksl::vec3 L = in.ctx->lights[i].position - in.position;
            float dist = ksl::length(L);
            L = L / ksl::max(dist, 0.0001f);
            ksl::vec3 H = ksl::normalize(V + L);

            float lint = in.ctx->lights[i].intensity;
            float lfalloff = ksl::max(in.ctx->lights[i].falloff, 0.0001f);
            float lcurve = in.ctx->lights[i].curve;
            float normDist = dist / lfalloff;
            float atten = lint / (1.0f + dist + lcurve * normDist * normDist);

            ksl::vec3 radiance = in.ctx->lights[i].color * atten;

            float NdotH = ksl::max(ksl::dot(N, H), 0.0f);
            float NdotL = ksl::max(ksl::dot(N, L), 0.0f);
            float NdotV = ksl::max(ksl::dot(N, V), 0.0001f);
            float HdotV = ksl::max(ksl::dot(H, V), 0.0f);

            // GGX Normal Distribution
            float denom = NdotH * NdotH * (a2 - 1.0f) + 1.0f;
            float D = a2 / (3.14159265f * denom * denom);

            // Fresnel-Schlick
            float ft = 1.0f - HdotV;
            float ft5 = ft * ft * ft * ft * ft;
            ksl::vec3 F = F0 + (ksl::vec3(1.0f) - F0) * ft5;

            // Smith geometry
            float k = (roughness + 1.0f);
            k = k * k / 8.0f;
            float G1v = NdotV / (NdotV * (1.0f - k) + k);
            float G1l = NdotL / (NdotL * (1.0f - k) + k);
            float G = G1v * G1l;

            ksl::vec3 numerator = F * D * G;
            float denomBRDF = 4.0f * NdotV * NdotL + 0.0001f;
            ksl::vec3 specular = numerator / denomBRDF;

            ksl::vec3 kD = (ksl::vec3(1.0f) - F) * (1.0f - metallic);
            Lo = Lo + (kD * albedo / 3.14159265f + specular) * radiance * NdotL;
        }

        ksl::vec3 ambient = ksl::vec3(0.03f) * albedo * ao;
        ksl::vec3 color = ambient + Lo + emissive;

        // Reinhard tone mapping + gamma
        color = color / (color + ksl::vec3(1.0f));
        color = ksl::pow(color, ksl::vec3(1.0f / 2.2f));

        return ksl::vec4(color, 1.0f);
    }
};
