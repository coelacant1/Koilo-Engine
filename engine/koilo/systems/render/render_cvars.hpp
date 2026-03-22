// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file render_cvars.hpp
 * @brief Console variables for the rendering subsystem.
 *
 * Declare as extern so any translation unit can read them without
 * re-registering. The single definition lives in render_cvars.cpp.
 *
 * @date 02/25/2026
 * @author Coela
 */
#pragma once

#include <koilo/kernel/cvar/cvar_system.hpp>

namespace koilo {

extern AutoCVar_Bool  cvar_r_wireframe;
extern AutoCVar_Bool  cvar_r_culling;
extern AutoCVar_Bool  cvar_r_normals;
extern AutoCVar_Int   cvar_r_maxfps;
extern AutoCVar_Bool  cvar_r_vsync;
extern AutoCVar_Bool  cvar_r_shadows;
extern AutoCVar_Bool  cvar_r_drawcalls;
extern AutoCVar_Bool  cvar_r_depthtest;
extern AutoCVar_Bool  cvar_r_legacy_backend;

} // namespace koilo
