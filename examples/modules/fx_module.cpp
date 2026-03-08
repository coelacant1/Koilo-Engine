// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file fx_module.cpp
 * @brief Demo ELF module: pixel buffer post-processing effects.
 *
 * Demonstrates the Koilo Engine external module render callback.
 * Applies effects (scanlines, tint, vignette) directly to the frame buffer.
 * The active effect is controlled via the fx_mode script variable:
 *   0 = off, 1 = scanlines, 2 = tint, 3 = vignette
 *
 * Build: g++ -shared -fpic -fno-rtti -fno-exceptions -O2
 *        -I engine/include -std=c++17 -o fx_module.so fx_module.cpp
 */

#include <koilo/modules/module_api.hpp>
#include <cmath>
#include <cstdint>

using namespace koilo;

// --- Module state ---
static EngineServices* svc = nullptr;
static void*           eng = nullptr;
static int activeMode = 0;

// Pixel layout matches koilo::Color888 (R, G, B bytes)
struct Pixel { uint8_t r, g, b; };

// --- Module header ---
static KoiloModuleHeader header = {
    KL_MODULE_MAGIC,
    KL_MODULE_ABI_VER,
    "fx",            // name
    "1.0.0",         // version
    static_cast<uint32_t>(ModulePhase::Render),
    0, {0, 0, 0, 0}
};

// --- Effect implementations ---

static void ApplyScanlines(Pixel* buf, int w, int h) {
    // Darken every other row by 40%
    for (int y = 0; y < h; y += 2) {
        Pixel* row = buf + y * w;
        for (int x = 0; x < w; ++x) {
            row[x].r = static_cast<uint8_t>(row[x].r * 0.6f);
            row[x].g = static_cast<uint8_t>(row[x].g * 0.6f);
            row[x].b = static_cast<uint8_t>(row[x].b * 0.6f);
        }
    }
}

static void ApplyTint(Pixel* buf, int w, int h) {
    // Warm orange tint: boost red, slight green, reduce blue
    for (int i = 0; i < w * h; ++i) {
        int r = static_cast<int>(buf[i].r * 1.2f);
        int g = static_cast<int>(buf[i].g * 0.9f);
        int b = static_cast<int>(buf[i].b * 0.6f);
        buf[i].r = static_cast<uint8_t>(r > 255 ? 255 : r);
        buf[i].g = static_cast<uint8_t>(g > 255 ? 255 : g);
        buf[i].b = static_cast<uint8_t>(b);
    }
}

static void ApplyVignette(Pixel* buf, int w, int h) {
    float cx = w * 0.5f;
    float cy = h * 0.5f;
    float maxDist = sqrtf(cx * cx + cy * cy);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float dx = x - cx;
            float dy = y - cy;
            float dist = sqrtf(dx * dx + dy * dy) / maxDist;
            float factor = 1.0f - dist * dist * 0.8f;
            if (factor < 0.0f) factor = 0.0f;
            Pixel& p = buf[y * w + x];
            p.r = static_cast<uint8_t>(p.r * factor);
            p.g = static_cast<uint8_t>(p.g * factor);
            p.b = static_cast<uint8_t>(p.b * factor);
        }
    }
}

// --- C ABI entry points ---

extern "C" {

KoiloModuleHeader* koilo_module_get_header() {
    return &header;
}

int koilo_module_init(EngineServices* services, void* engine) {
    svc = services;
    eng = engine;
    activeMode = 0;
    svc->set_variable(eng, "fx_mode", 0.0);
    return 1;
}

void koilo_module_update(float /*dt*/) {
    if (svc) {
        activeMode = static_cast<int>(svc->get_variable(eng, "fx_mode"));
    }
}

void koilo_module_render(void* buffer, int width, int height) {
    if (!buffer || width <= 0 || height <= 0) return;
    auto* px = static_cast<Pixel*>(buffer);

    switch (activeMode) {
        case 1: ApplyScanlines(px, width, height); break;
        case 2: ApplyTint(px, width, height);      break;
        case 3: ApplyVignette(px, width, height);   break;
        default: break;  // mode 0 = no effect
    }
}

void koilo_module_shutdown() {
    svc = nullptr;
    eng = nullptr;
}

} // extern "C"
