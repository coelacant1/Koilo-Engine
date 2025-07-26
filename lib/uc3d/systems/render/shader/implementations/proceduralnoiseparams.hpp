#pragma once

#include "../../../../core/math/vector3d.hpp"
#include "../../../../core/color/rgbcolor.hpp"

struct ProceduralNoiseParams {
    static constexpr std::size_t kColors = 4;         // gradient resolution

    RGBColor spectrum[kColors] = {                    // default = black-white stripes
        {0,0,0}, {255,255,255}, {0,0,0}, {255,255,255}
    };

    Vector3D noiseScale        {0.01f, 0.01f, 0.01f}; // freq per axis
    float    simplexDepth      = 0.0f;                // Z slice for time-animation
    float    gradientPeriod    = 1.0f;                // repeat every N noise units
    float    hueShiftAngleDeg  = 0.0f;                // rainbow phase-shift
};
