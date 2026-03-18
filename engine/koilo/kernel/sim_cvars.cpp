// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file sim_cvars.cpp
 * @brief Simulation control CVar definitions.
 *
 * @date 01/18/2026
 * @author Coela Can't
 */

#include "sim_cvars.hpp"

namespace koilo {

AutoCVar_Bool cvar_sim_pause{"sim.pause", "Pause simulation ticking", false};

std::atomic<int> g_simStepFrames{0};

// Force-link anchor so static libraries include this TU.
extern "C" void koilo_sim_cvars_anchor() {}

} // namespace koilo
