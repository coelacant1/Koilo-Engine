// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file contracts.hpp
 * @brief Design-by-contract macros: KL_REQUIRE, KL_ENSURE, KL_INVARIANT.
 *
 * Preconditions (KL_REQUIRE) express what the caller must guarantee.
 * Postconditions (KL_ENSURE) express what the callee guarantees on return.
 * Invariants (KL_INVARIANT) express what must always hold for a class/loop.
 *
 * Behavior by build configuration:
 *   Debug / RelWithDebInfo (NDEBUG not defined):
 *     - Structured log via KL_ERR
 *     - Post MSG_CONTRACT_VIOLATION to MessageBus (if available)
 *     - assert(false) to halt in debugger
 *
 *   Release (NDEBUG defined):
 *     - Entirely stripped (zero overhead)
 *     - Optionally expands to __builtin_unreachable() for optimizer hints
 *
 * @date 03/30/2026
 * @author Coela
 */
#pragma once

#include "logging/log.hpp"
#include "message_bus.hpp"

#include <cassert>
#include <cstring>

namespace koilo {

/// Contract violation kinds, used as payload in MSG_CONTRACT_VIOLATION.
enum class ContractKind : uint8_t {
    Require,    ///< Precondition (caller responsibility)
    Ensure,     ///< Postcondition (callee guarantee)
    Invariant   ///< Class/loop invariant
};

/// Payload dispatched via MessageBus on contract violation.
struct ContractViolation {
    ContractKind kind;
    const char*  expression;  ///< Stringified condition
    const char*  message;     ///< Developer message
    const char*  file;
    int          line;
};

inline const char* ContractKindName(ContractKind k) {
    switch (k) {
        case ContractKind::Require:   return "REQUIRE";
        case ContractKind::Ensure:    return "ENSURE";
        case ContractKind::Invariant: return "INVARIANT";
        default:                      return "UNKNOWN";
    }
}

/// Internal handler - do not call directly; use KL_REQUIRE / KL_ENSURE / KL_INVARIANT.
[[noreturn]] inline void ContractFail(ContractKind kind,
                                      const char* expr,
                                      const char* msg,
                                      const char* file,
                                      int line) {
    // Structured log
    KL_ERR("Contract", "%s violation: %s | %s (%s:%d)",
           ContractKindName(kind), expr, msg, file, line);

    // Dispatch to MessageBus if available
    MessageBus* bus = GetMessageBus();
    if (bus) {
        ContractViolation v{kind, expr, msg, file, line};
        bus->SendImmediate(
            MakeMessage(MSG_CONTRACT_VIOLATION, MODULE_KERNEL, v));
    }

    // Break into debugger / abort
    assert(false && "Contract violation - see log above");

    // If assert is somehow a no-op (should not happen in debug), trap anyway
#if defined(__GNUC__) || defined(__clang__)
    __builtin_unreachable();
#elif defined(_MSC_VER)
    __assume(false);
#endif
}

} // namespace koilo

// ---- Public macros ----

#ifndef NDEBUG

/// Precondition - caller must guarantee this before calling.
#define KL_REQUIRE(expr, msg)                                          \
    do {                                                               \
        if (!(expr)) {                                                 \
            ::koilo::ContractFail(::koilo::ContractKind::Require,      \
                                  #expr, (msg), __FILE__, __LINE__);   \
        }                                                              \
    } while (0)

/// Postcondition - callee guarantees this on return.
#define KL_ENSURE(expr, msg)                                           \
    do {                                                               \
        if (!(expr)) {                                                 \
            ::koilo::ContractFail(::koilo::ContractKind::Ensure,       \
                                  #expr, (msg), __FILE__, __LINE__);   \
        }                                                              \
    } while (0)

/// Invariant - must hold at this point (class invariant, loop invariant, etc.).
#define KL_INVARIANT(expr, msg)                                        \
    do {                                                               \
        if (!(expr)) {                                                 \
            ::koilo::ContractFail(::koilo::ContractKind::Invariant,    \
                                  #expr, (msg), __FILE__, __LINE__);   \
        }                                                              \
    } while (0)

#else // NDEBUG - Release builds: zero overhead

#if defined(__GNUC__) || defined(__clang__)
    #define KL_REQUIRE(expr, msg)   do { if (!(expr)) __builtin_unreachable(); } while (0)
    #define KL_ENSURE(expr, msg)    do { if (!(expr)) __builtin_unreachable(); } while (0)
    #define KL_INVARIANT(expr, msg) do { if (!(expr)) __builtin_unreachable(); } while (0)
#elif defined(_MSC_VER)
    #define KL_REQUIRE(expr, msg)   __assume(expr)
    #define KL_ENSURE(expr, msg)    __assume(expr)
    #define KL_INVARIANT(expr, msg) __assume(expr)
#else
    #define KL_REQUIRE(expr, msg)   ((void)0)
    #define KL_ENSURE(expr, msg)    ((void)0)
    #define KL_INVARIANT(expr, msg) ((void)0)
#endif

#endif // NDEBUG
