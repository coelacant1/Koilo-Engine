// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file deprecation.hpp
 * @brief Deprecation macros for the Koilo Engine.
 *
 * Usage:
 *   KL_DEPRECATED("Use NewFunction() instead")
 *   void OldFunction();
 *
 *   KL_DEPRECATED_SINCE(0, 4, 0, "Replaced by NewWidget")
 *   class OldWidget { ... };
 *
 * @date 03/30/2026
 * @author Coela
 */
#pragma once

/// Mark a symbol as deprecated with a message.
#if defined(__has_cpp_attribute) && __has_cpp_attribute(deprecated)
  #define KL_DEPRECATED(msg)  [[deprecated(msg)]]
#elif defined(__GNUC__) || defined(__clang__)
  #define KL_DEPRECATED(msg)  __attribute__((deprecated(msg)))
#elif defined(_MSC_VER)
  #define KL_DEPRECATED(msg)  __declspec(deprecated(msg))
#else
  #define KL_DEPRECATED(msg)
#endif

/// Mark a symbol as deprecated since a specific version.
/// Example: KL_DEPRECATED_SINCE(0, 4, 0, "Use X instead")
#define KL_DEPRECATED_SINCE(major, minor, patch, msg) \
    KL_DEPRECATED("Since " #major "." #minor "." #patch ": " msg)
