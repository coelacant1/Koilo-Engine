// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file game_mode_module.cpp
 * @brief Demo ELF module: game state management (score, lives, timer).
 *
 * Demonstrates the Koilo Engine external module C ABI for gameplay modding.
 * Exposes game_score, game_lives, game_timer, game_over as script variables.
 * Scripts write game_add_score / game_lose_life to trigger state changes.
 *
 * Build: g++ -shared -fpic -fno-rtti -fno-exceptions -O2
 *        -I engine/include -std=c++17 -o game_mode_module.so game_mode_module.cpp
 */

#include <koilo/modules/module_api.hpp>

using namespace koilo;

// --- Module state ---
static EngineServices* svc = nullptr;
static void*           eng = nullptr;

static double score    = 0.0;
static double lives    = 3.0;
static double timer    = 0.0;
static double gameOver = 0.0;

// --- Module header ---
static KoiloModuleHeader header = {
    KL_MODULE_MAGIC,
    KL_MODULE_ABI_VER,
    "game_mode",     // name
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
    score    = 0.0;
    lives    = 3.0;
    timer    = 0.0;
    gameOver = 0.0;
    svc->set_variable(eng, "game_score",    score);
    svc->set_variable(eng, "game_lives",    lives);
    svc->set_variable(eng, "game_timer",    timer);
    svc->set_variable(eng, "game_over",     gameOver);
    svc->set_variable(eng, "game_add_score", 0.0);
    svc->set_variable(eng, "game_lose_life", 0.0);
    svc->set_variable(eng, "game_reset",     0.0);
    return 1;
}

void koilo_module_update(float dt) {
    if (!svc || gameOver > 0.0) return;

    // Check for script-driven commands
    double addScore = svc->get_variable(eng, "game_add_score");
    if (addScore > 0.0) {
        score += addScore;
        svc->set_variable(eng, "game_add_score", 0.0);
    }

    double losLife = svc->get_variable(eng, "game_lose_life");
    if (losLife > 0.0) {
        lives -= 1.0;
        svc->set_variable(eng, "game_lose_life", 0.0);
        if (lives <= 0.0) {
            lives = 0.0;
            gameOver = 1.0;
        }
    }

    double reset = svc->get_variable(eng, "game_reset");
    if (reset > 0.0) {
        score    = 0.0;
        lives    = 3.0;
        timer    = 0.0;
        gameOver = 0.0;
        svc->set_variable(eng, "game_reset", 0.0);
    }

    // Advance timer
    timer += static_cast<double>(dt);

    // Push state to script
    svc->set_variable(eng, "game_score", score);
    svc->set_variable(eng, "game_lives", lives);
    svc->set_variable(eng, "game_timer", timer);
    svc->set_variable(eng, "game_over",  gameOver);
}

void koilo_module_render(void* /*buffer*/, int /*width*/, int /*height*/) {
    // Game mode has no visual output
}

void koilo_module_shutdown() {
    svc = nullptr;
    eng = nullptr;
}

} // extern "C"
