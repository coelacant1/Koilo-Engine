// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file module_abi_adapters.hpp
 * @brief C ABI v3 adapter functions that bridge C descriptors to C++ subsystems.
 *
 * Each adapter takes the opaque engine pointer (KoiloKernel*) and a C descriptor,
 * then registers the described resource with the appropriate C++ subsystem.
 *
 * @date 03/30/2026
 * @author Coela
 */
#pragma once

#include "module_api.hpp"

namespace koilo {

class KoiloKernel;

/// Register a console command from a C ABI descriptor.
int AbiRegisterCommand(void* engine, const KoiloCommandDesc* desc);

/// Register an input listener from a C ABI descriptor.
int AbiRegisterInputListener(void* engine, const KoiloInputListenerDesc* desc);

/// Register a component type from a C ABI descriptor.
int AbiRegisterComponent(void* engine, const KoiloComponentDesc* desc);

/// Register a widget type from a C ABI descriptor.
int AbiRegisterWidgetType(void* engine, const KoiloWidgetTypeDesc* desc);

/// Register a render pass from a C ABI descriptor.
int AbiRegisterRenderPass(void* engine, const KoiloRenderPassDesc* desc);

} // namespace koilo
