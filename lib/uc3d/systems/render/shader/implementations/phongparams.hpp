#pragma once

#include "../../../../core/color/rgbcolor.hpp"

struct PhongParams {
    RGBColor ambientColor   { 20, 20, 20 };       // Color from ambient light
    RGBColor diffuseColor   { 128, 128, 128 };    // Main surface color (albedo)
    RGBColor specularColor  { 255, 255, 255 };    // Highlight color
    float    shininess      { 32.0f };            // Highlight size (higher is smaller/sharper)
};
