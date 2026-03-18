// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file sim_cvars.hpp
 * @brief Console variables for simulation control (pause, step).
 *
 * @date 01/18/2026
 * @author Coela Can't
 */
#pragma once

#include <koilo/kernel/cvar/cvar_system.hpp>
#include <atomic>

namespace koilo {

extern AutoCVar_Bool cvar_sim_pause;

/// Atomic step counter - when > 0, the frame loop advances one frame and decrements.
/// Separate from CVar to avoid overhead on the hot path.
extern std::atomic<int> g_simStepFrames;

} // namespace koilo
