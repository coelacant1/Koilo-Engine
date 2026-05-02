// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file stability.hpp
 * @brief API stability tier annotations.
 *
 * Every public API surface should carry a stability annotation so users
 * and tooling know what compatibility guarantees apply.
 *
 * Tiers (from most to least stable):
 *   KL_FROZEN    -- Never changes. C ABI struct layouts, magic constants,
 *                  module header layout. Breaking these breaks all modules.
 *   KL_STABLE    -- Changes only across major versions. Core interfaces,
 *                  ServiceRegistry API, MessageBus API.
 *   KL_UNSTABLE  -- May change in minor versions. Experimental features,
 *                  new extension points under active development.
 *   KL_INTERNAL  -- May change in any release. Backends, build system,
 *                  private implementation details.
 *
 * Usage:
 *   KL_FROZEN struct EngineServices { ... };
 *   KL_STABLE class MessageBus { ... };
 *   KL_UNSTABLE void ExperimentalFeature();
 *
 * The macros expand to [[gnu::annotate(...)]] where supported, or to
 * nothing on compilers that don't support it. The abi_check.py tool
 * parses these annotations from source to build the freeze baseline.
 *
 * @date 03/30/2026
 * @author Coela
 */
#pragma once

// Stability tier markers -- used by tooling (abi_check.py) and documentation.
// abi_check.py parses the macro tokens from source text directly, so these
// only need to expand to a real attribute on toolchains that actually consume
// the annotation through libclang. GCC ignores [[gnu::annotate]] on most
// type-position uses (enums, structs) and emits a -Wattributes warning for
// every occurrence, so we restrict the attribute form to Clang.

#if defined(__clang__)
  #define KL_FROZEN    __attribute__((annotate("koilo:frozen")))
  #define KL_STABLE    __attribute__((annotate("koilo:stable")))
  #define KL_UNSTABLE  __attribute__((annotate("koilo:unstable")))
  #define KL_INTERNAL  __attribute__((annotate("koilo:internal")))
#else
  #define KL_FROZEN
  #define KL_STABLE
  #define KL_UNSTABLE
  #define KL_INTERNAL
#endif
