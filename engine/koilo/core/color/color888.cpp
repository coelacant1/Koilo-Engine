// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file color888.cpp
 * @brief Implementation of Color888 class.
 *
 * @date 11/10/2025
 * @author Coela
 */

#include <koilo/core/color/color888.hpp>
#include <koilo/core/math/quaternion.hpp>
#include <cmath>

namespace koilo {

Color888 koilo::Color888::HueShift(float hueDeg) const {
    // Convert RGB to HSV
    float r_norm = r / 255.0f;
    float g_norm = g / 255.0f;
    float b_norm = b / 255.0f;
    
    float max = Mathematics::Max(r_norm, Mathematics::Max(g_norm, b_norm));
    float min = Mathematics::Min(r_norm, Mathematics::Min(g_norm, b_norm));
    float delta = max - min;
    
    float h = 0.0f, s = 0.0f, v = max;
    
    if (delta > 0.00001f) {
        s = delta / max;
        
        if (r_norm == max) {
            h = 60.0f * (g_norm - b_norm) / delta;
        } else if (g_norm == max) {
            h = 60.0f * (2.0f + (b_norm - r_norm) / delta);
        } else {
            h = 60.0f * (4.0f + (r_norm - g_norm) / delta);
        }
        
        if (h < 0.0f) h += 360.0f;
    }
    
    // Apply hue shift
    h += hueDeg;
    while (h >= 360.0f) h -= 360.0f;
    while (h < 0.0f) h += 360.0f;
    
    // Convert back to RGB
    float c = v * s;
    float x = c * (1.0f - Mathematics::FAbs(fmod(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;
    
    float r1 = 0, g1 = 0, b1 = 0;
    
    if (h < 60.0f) {
        r1 = c; g1 = x; b1 = 0;
    } else if (h < 120.0f) {
        r1 = x; g1 = c; b1 = 0;
    } else if (h < 180.0f) {
        r1 = 0; g1 = c; b1 = x;
    } else if (h < 240.0f) {
        r1 = 0; g1 = x; b1 = c;
    } else if (h < 300.0f) {
        r1 = x; g1 = 0; b1 = c;
    } else {
        r1 = c; g1 = 0; b1 = x;
    }
    
    return Color888(
        static_cast<uint8_t>((r1 + m) * 255.0f),
        static_cast<uint8_t>((g1 + m) * 255.0f),
        static_cast<uint8_t>((b1 + m) * 255.0f)
    );
}

Color888 koilo::Color888::FromHSV(float h, float s, float v) {
    s = Mathematics::Constrain(s, 0.0f, 1.0f);
    v = Mathematics::Constrain(v, 0.0f, 1.0f);
    while (h >= 360.0f) h -= 360.0f;
    while (h < 0.0f) h += 360.0f;

    float c = v * s;
    float x = c * (1.0f - Mathematics::FAbs(fmod(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;

    float r1 = 0, g1 = 0, b1 = 0;

    if (h < 60.0f) {
        r1 = c; g1 = x; b1 = 0;
    } else if (h < 120.0f) {
        r1 = x; g1 = c; b1 = 0;
    } else if (h < 180.0f) {
        r1 = 0; g1 = c; b1 = x;
    } else if (h < 240.0f) {
        r1 = 0; g1 = x; b1 = c;
    } else if (h < 300.0f) {
        r1 = x; g1 = 0; b1 = c;
    } else {
        r1 = c; g1 = 0; b1 = x;
    }

    return Color888(
        static_cast<uint8_t>((r1 + m) * 255.0f),
        static_cast<uint8_t>((g1 + m) * 255.0f),
        static_cast<uint8_t>((b1 + m) * 255.0f)
    );
}

} // namespace koilo
