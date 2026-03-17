// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file register_services.hpp
 * @brief Core service registration at kernel startup.
 *
 * @date 02/03/2026
 * @author Coela
 */
#pragma once

namespace koilo {

class KoiloKernel;

/// Register well-known singletons as kernel services.
/// Call once after KoiloKernel construction.
void RegisterCoreServices(KoiloKernel& kernel);

} // namespace koilo
