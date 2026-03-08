// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file sensor_module.cpp
 * @brief Demo ELF module: simulated hardware sensor data.
 *
 * Demonstrates the Koilo Engine external module C ABI.
 * Exposes sensor_temperature, sensor_distance, sensor_light as script
 * variables updated each frame with simulated noise.
 *
 * Build: g++ -shared -fpic -fno-rtti -fno-exceptions -O2
 *        -I engine/include -std=c++17 -o sensor_module.so sensor_module.cpp
 */

#include <koilo/modules/module_api.hpp>
#include <cmath>

using namespace koilo;

// --- Module state ---
static EngineServices* svc = nullptr;
static void*           eng = nullptr;

static float temperature = 22.0f;
static float distance    = 100.0f;
static float light       = 0.5f;
static float elapsed     = 0.0f;

// Simple deterministic pseudo-noise (no stdlib rand dependency)
static float noise(float seed) {
    int s = static_cast<int>(seed * 1000.0f) ^ 0xDEAD;
    s = (s * 1103515245 + 12345) & 0x7FFFFFFF;
    return (static_cast<float>(s) / 0x7FFFFFFF) * 2.0f - 1.0f;
}

// --- Module header ---
static KoiloModuleHeader header = {
    KL_MODULE_MAGIC,
    KL_MODULE_ABI_VER,
    "sensor",        // name
    "1.0.0",         // version
    static_cast<uint32_t>(ModulePhase::System),
    0, {0, 0, 0, 0}
};

// --- C ABI entry points ---

extern "C" {

KoiloModuleHeader* koilo_module_get_header() {
    return &header;
}

int koilo_module_init(EngineServices* services, void* engine) {
    svc = services;
    eng = engine;
    // Set initial values
    svc->set_variable(eng, "sensor_temperature", temperature);
    svc->set_variable(eng, "sensor_distance",    distance);
    svc->set_variable(eng, "sensor_light",       light);
    return 1;
}

void koilo_module_update(float dt) {
    elapsed += dt;

    // Simulate temperature: ambient 22°C with slow drift and noise
    temperature = 22.0f + 3.0f * sinf(elapsed * 0.1f) + noise(elapsed) * 0.5f;

    // Simulate distance: object oscillating 50-150cm
    distance = 100.0f + 50.0f * sinf(elapsed * 0.5f) + noise(elapsed + 100.0f) * 5.0f;

    // Simulate light: day/night cycle 0-1
    light = 0.5f + 0.5f * sinf(elapsed * 0.05f);

    if (svc) {
        svc->set_variable(eng, "sensor_temperature", static_cast<double>(temperature));
        svc->set_variable(eng, "sensor_distance",    static_cast<double>(distance));
        svc->set_variable(eng, "sensor_light",       static_cast<double>(light));
    }
}

void koilo_module_render(void* /*buffer*/, int /*width*/, int /*height*/) {
    // Sensor module has no visual output
}

void koilo_module_shutdown() {
    svc = nullptr;
    eng = nullptr;
}

} // extern "C"
