// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <memory>
#include <koilo/core/color/color888.hpp>
#include <koilo/core/math/vector3d.hpp>
#include <koilo/systems/render/material/implementations/kslmaterial.hpp>
#include <koilo/registry/reflect_macros.hpp>

namespace koilo {

class Sky {
public:
    Sky() = default;

    static Sky& GetInstance();

    // --- Control ---
    void Enable();
    void Disable() { enabled_ = false; }
    bool IsEnabled() const { return enabled_; }

    void SetTimeOfDay(float hours);
    float GetTimeOfDay() const { return timeOfDay_; }

    /// Time speed in game-hours per real-second (default 0.5 -> 2 real min = 1 game hour)
    void SetTimeSpeed(float speed) { timeSpeed_ = speed; }
    float GetTimeSpeed() const { return timeSpeed_; }

    void Update(float dt);

    // --- Sky material (KSL-based sky shader) ---
    void SetMaterial(KSLMaterial* mat) { skyMaterial_ = mat; }
    KSLMaterial* GetMaterial() const { return skyMaterial_ ? skyMaterial_ : defaultMaterial_.get(); }
    bool HasMaterial() const { return skyMaterial_ != nullptr || defaultMaterial_ != nullptr; }

    // --- Sky color queries ---
    Color888 GetTopColor() const;
    Color888 GetBottomColor() const;

    // --- Celestial body directions (world space, Y-up) ---
    Vector3D GetSunDirection() const;
    Vector3D GetMoonDirection() const;

    float GetSunScreenRadius()  const { return 4.0f; }
    float GetMoonScreenRadius() const { return 3.0f; }

    Color888 GetSunColor() const;
    Color888 GetMoonColor() const;

    Color888 SamplePixel(int px, int py, int w, int h,
                         int sunX, int sunY, bool sunVis,
                         int moonX, int moonY, bool moonVis) const;

private:
    float timeOfDay_ = 12.0f;
    float timeSpeed_ = 0.5f;
    bool  enabled_   = false;
    KSLMaterial* skyMaterial_ = nullptr;             // user-set (borrowed)
    std::unique_ptr<KSLMaterial> defaultMaterial_;   // auto-created fallback

    static Sky* instance_;

    // Keyframe interpolation
    struct SkyKey {
        float hour;
        uint8_t topR, topG, topB;
        uint8_t botR, botG, botB;

        KL_BEGIN_FIELDS(SkyKey)
            KL_FIELD(SkyKey, hour, "Hour", __FLT_MIN__, __FLT_MAX__),
            KL_FIELD(SkyKey, topR, "Top r", 0, 255),
            KL_FIELD(SkyKey, topG, "Top g", 0, 255),
            KL_FIELD(SkyKey, topB, "Top b", 0, 255),
            KL_FIELD(SkyKey, botR, "Bot r", 0, 255),
            KL_FIELD(SkyKey, botG, "Bot g", 0, 255),
            KL_FIELD(SkyKey, botB, "Bot b", 0, 255)
        KL_END_FIELDS

        KL_BEGIN_METHODS(SkyKey)
            /* No reflected methods. */
        KL_END_METHODS

        KL_BEGIN_DESCRIBE(SkyKey)
            /* No reflected ctors. */
        KL_END_DESCRIBE(SkyKey)

    };

    static const SkyKey keyframes_[];
    static const int    keyframeCount_;

    static uint8_t lerp8(uint8_t a, uint8_t b, float t) {
        return static_cast<uint8_t>(a + (b - a) * t);
    }

    KL_BEGIN_FIELDS(Sky)
    KL_END_FIELDS

    KL_BEGIN_METHODS(Sky)
        KL_METHOD_AUTO(Sky, Enable,        "Enable"),
        KL_METHOD_AUTO(Sky, Disable,       "Disable"),
        KL_METHOD_AUTO(Sky, IsEnabled,     "Is enabled"),
        KL_METHOD_AUTO(Sky, SetTimeOfDay,  "Set time of day"),
        KL_METHOD_AUTO(Sky, GetTimeOfDay,  "Get time of day"),
        KL_METHOD_AUTO(Sky, SetTimeSpeed,  "Set time speed"),
        KL_METHOD_AUTO(Sky, GetTimeSpeed,  "Get time speed"),
        KL_METHOD_AUTO(Sky, SetMaterial,   "Set sky material"),
        KL_METHOD_AUTO(Sky, HasMaterial,   "Has sky material")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(Sky)
    KL_END_DESCRIBE(Sky)
};

} // namespace koilo
