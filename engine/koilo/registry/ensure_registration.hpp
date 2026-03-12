// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

/**
 * @file ensure_registration.hpp
 * @brief Force linking of reflection_entry_gen.cpp to enable automatic registration
 * 
 * Problem: Static initializers in reflection_entry_gen.cpp may not run if no symbols
 * from that translation unit are referenced by the linker.
 * 
 * Solution: Provide a function that programs can call to ensure the TU is linked.
 */

namespace koilo {
namespace registry {

/**
 * @brief Ensure all reflected classes are registered
 * 
 * Call this function early in main() to ensure all classes with KL_BEGIN_DESCRIBE
 * are registered in the global registry. This forces the linker to include
 * reflection_entry_gen.cpp, which contains static initializers that call Describe()
 * on all reflected classes.
 * 
 * Note: This function does nothing at runtime - it just needs to exist so the
 * linker pulls in reflection_entry_gen.cpp.
 */
void EnsureReflectionRegistered();

/**
 * @brief Ensure core (non-platform) reflected classes are registered.
 * Safe for headless builds - does not pull in display backends.
 */
void EnsureCoreReflectionRegistered();

/**
 * @brief Ensure platform-specific reflected classes are registered.
 * Call from hosts that link display backends (SDL3, OpenGL, HUB75, etc.).
 */
void EnsurePlatformReflectionRegistered();

} // namespace registry
} // namespace koilo
