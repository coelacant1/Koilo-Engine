// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file render_cvars.cpp
 * @brief Render CVar definitions (self-register before main).
 *
 * @date 02/25/2026
 * @author Coela
 */
#include <koilo/systems/render/render_cvars.hpp>

namespace koilo {

AutoCVar_Bool  cvar_r_wireframe {"r.wireframe",  "Render in wireframe mode",          false};
AutoCVar_Bool  cvar_r_culling   {"r.culling",    "Enable backface culling",            true};
AutoCVar_Bool  cvar_r_normals   {"r.normals",    "Visualise surface normals",          false};
AutoCVar_Int   cvar_r_maxfps    {"r.maxfps",     "Frame rate cap (0 = uncapped)",      60};
AutoCVar_Bool  cvar_r_vsync     {"r.vsync",      "Enable vertical sync",               false};
AutoCVar_Bool  cvar_r_shadows   {"r.shadows",    "Enable shadow rendering",            true};
AutoCVar_Bool  cvar_r_drawcalls {"r.drawcalls",  "Show draw call count in HUD",        false};
AutoCVar_Bool  cvar_r_depthtest {"r.depthtest",  "Enable depth testing",               true};

} // namespace koilo

// Force-link: referenced from console_module to ensure static CVars register.
extern "C" void koilo_render_cvars_anchor() {}
