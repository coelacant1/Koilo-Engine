// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/render/sky/sky.hpp>
#include <cmath>
#include <algorithm>

namespace koilo {

Sky& Sky::GetInstance() {
    if (s_instance) return *s_instance;
    static Sky fallback;
    return fallback;
}

void Sky::Enable() {
    enabled_ = true;
    // Auto-create default KSL sky material if user hasn't set one
    if (!skyMaterial_ && !defaultMaterial_) {
        defaultMaterial_ = std::make_unique<KSLMaterial>("sky");
    }
}

void Sky::SetTimeOfDay(float hours) {
    timeOfDay_ = std::fmod(hours, 24.0f);
    if (timeOfDay_ < 0) timeOfDay_ += 24.0f;
}

void Sky::Update(float dt) {
    if (!enabled_) return;
    timeOfDay_ = std::fmod(timeOfDay_ + timeSpeed_ * dt, 24.0f);
    if (timeOfDay_ < 0) timeOfDay_ += 24.0f;

    // Push time-of-day colors into the active sky material
    KSLMaterial* mat = GetMaterial();
    if (mat) {
        Color888 top = GetTopColor();
        Color888 bot = GetBottomColor();
        mat->SetVec3("topColor",
            top.R / 255.0f, top.G / 255.0f, top.B / 255.0f);
        mat->SetVec3("bottomColor",
            bot.R / 255.0f, bot.G / 255.0f, bot.B / 255.0f);
    }
}

// --- Color keyframes (smooth time-of-day transitions) ---
const Sky::SkyKey Sky::keyframes_[] = {
    //  hour   topR topG topB   botR botG botB
    {  0.0f,    5,   8,  20,     2,   3,   8 },   // midnight
    {  5.0f,    8,  10,  25,     5,   5,  12 },   // late night
    {  6.0f,   35,  25,  55,   140,  75,  45 },   // dawn
    {  7.5f,   90, 135, 200,   175, 155, 135 },   // morning
    { 12.0f,   65, 135, 215,   165, 195, 225 },   // midday
    { 16.5f,   90, 130, 195,   175, 165, 145 },   // afternoon
    { 18.0f,   45,  22,  65,   195,  85,  45 },   // dusk
    { 19.5f,   12,   8,  30,    35,  18,  22 },   // twilight
    { 24.0f,    5,   8,  20,     2,   3,   8 },   // midnight wrap
};
const int Sky::keyframeCount_ = sizeof(keyframes_) / sizeof(keyframes_[0]);

Color888 Sky::GetTopColor() const {
    float h = timeOfDay_;
    for (int i = 0; i < keyframeCount_ - 1; ++i) {
        if (h >= keyframes_[i].hour && h <= keyframes_[i + 1].hour) {
            float t = (h - keyframes_[i].hour) /
                      (keyframes_[i + 1].hour - keyframes_[i].hour);
            return Color888(
                lerp8(keyframes_[i].topR, keyframes_[i + 1].topR, t),
                lerp8(keyframes_[i].topG, keyframes_[i + 1].topG, t),
                lerp8(keyframes_[i].topB, keyframes_[i + 1].topB, t));
        }
    }
    return Color888(5, 8, 20);
}

Color888 Sky::GetBottomColor() const {
    float h = timeOfDay_;
    for (int i = 0; i < keyframeCount_ - 1; ++i) {
        if (h >= keyframes_[i].hour && h <= keyframes_[i + 1].hour) {
            float t = (h - keyframes_[i].hour) /
                      (keyframes_[i + 1].hour - keyframes_[i].hour);
            return Color888(
                lerp8(keyframes_[i].botR, keyframes_[i + 1].botR, t),
                lerp8(keyframes_[i].botG, keyframes_[i + 1].botG, t),
                lerp8(keyframes_[i].botB, keyframes_[i + 1].botB, t));
        }
    }
    return Color888(2, 3, 8);
}

Vector3D Sky::GetSunDirection() const {
    // Sun rises East (+X) at 6, zenith at 12, sets West (-X) at 18
    float hourAngle = (timeOfDay_ - 12.0f) / 12.0f * 3.14159265f;
    float elevation = std::sin((timeOfDay_ - 6.0f) / 12.0f * 3.14159265f);
    float ce = std::sqrt(1.0f - elevation * elevation);
    return Vector3D(-std::sin(hourAngle) * ce,
                     elevation,
                    -std::cos(hourAngle) * ce);
}

Vector3D Sky::GetMoonDirection() const {
    // Moon is 12 hours offset from sun
    float moonTime = std::fmod(timeOfDay_ + 12.0f, 24.0f);
    float hourAngle = (moonTime - 12.0f) / 12.0f * 3.14159265f;
    float elevation = std::sin((moonTime - 6.0f) / 12.0f * 3.14159265f);
    float ce = std::sqrt(1.0f - elevation * elevation);
    return Vector3D(-std::sin(hourAngle) * ce,
                     elevation,
                    -std::cos(hourAngle) * ce);
}

Color888 Sky::GetSunColor() const {
    // Warm orange near horizon, bright white-yellow at zenith
    float elev = std::sin((timeOfDay_ - 6.0f) / 12.0f * 3.14159265f);
    if (elev <= 0) return Color888(0, 0, 0);
    float t = std::min(elev * 2.0f, 1.0f); // 0 at horizon, 1 at high
    return Color888(255,
                    lerp8(160, 245, t),
                    lerp8(60, 210, t));
}

Color888 Sky::GetMoonColor() const {
    return Color888(195, 205, 225);
}

Color888 Sky::SamplePixel(int px, int py, int w, int h,
                          int sunX, int sunY, bool sunVis,
                          int moonX, int moonY, bool moonVis) const {
    // Vertical gradient
    float vt = static_cast<float>(py) / static_cast<float>(h > 1 ? h - 1 : 1);
    Color888 top = GetTopColor();
    Color888 bot = GetBottomColor();
    int r = top.R + static_cast<int>((bot.R - top.R) * vt);
    int g = top.G + static_cast<int>((bot.G - top.G) * vt);
    int b = top.B + static_cast<int>((bot.B - top.B) * vt);

    // Sun disc + glow
    if (sunVis) {
        float dx = static_cast<float>(px - sunX);
        float dy = static_cast<float>(py - sunY);
        float dist = std::sqrt(dx * dx + dy * dy);
        float sunR = GetSunScreenRadius();
        Color888 sc = GetSunColor();
        if (dist < sunR) {
            return sc;
        } else if (dist < sunR * 3.0f) {
            float glow = 1.0f - (dist - sunR) / (sunR * 2.0f);
            glow *= glow;
            r = std::min(255, r + static_cast<int>(sc.R * glow * 0.5f));
            g = std::min(255, g + static_cast<int>(sc.G * glow * 0.4f));
            b = std::min(255, b + static_cast<int>(sc.B * glow * 0.3f));
        }
    }

    // Moon disc
    if (moonVis) {
        float dx = static_cast<float>(px - moonX);
        float dy = static_cast<float>(py - moonY);
        float dist = std::sqrt(dx * dx + dy * dy);
        if (dist < GetMoonScreenRadius()) {
            return GetMoonColor();
        } else if (dist < GetMoonScreenRadius() * 2.0f) {
            float glow = 1.0f - (dist - GetMoonScreenRadius()) / GetMoonScreenRadius();
            glow *= glow;
            Color888 mc = GetMoonColor();
            r = std::min(255, r + static_cast<int>(mc.R * glow * 0.2f));
            g = std::min(255, g + static_cast<int>(mc.G * glow * 0.2f));
            b = std::min(255, b + static_cast<int>(mc.B * glow * 0.25f));
        }
    }

    return Color888(static_cast<uint8_t>(std::max(0, std::min(255, r))),
                    static_cast<uint8_t>(std::max(0, std::min(255, g))),
                    static_cast<uint8_t>(std::max(0, std::min(255, b))));
}

} // namespace koilo
