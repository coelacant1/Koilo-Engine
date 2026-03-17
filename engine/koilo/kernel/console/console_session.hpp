// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file console_session.hpp
 * @brief Console session managing history, aliases, and command execution.
 *
 * @date 12/17/2025
 * @author Coela
 */
#pragma once
#include <koilo/kernel/console/console_result.hpp>
#include <koilo/kernel/console/command_registry.hpp>
#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <functional>
#include "../../registry/reflect_macros.hpp"

namespace koilo {

class KoiloKernel;

/// Console session: manages history, aliases, output format, and command execution.
class ConsoleSession {
public:
    using OutputCallback = std::function<void(const std::string&)>;

    explicit ConsoleSession(KoiloKernel& kernel, CommandRegistry& registry);

    /// Process a line of input. Returns the result.
    ConsoleResult Execute(const std::string& input);

    /// Get tab-completion candidates for partial input.
    std::vector<std::string> Complete(const std::string& input) const;

    // --- History ---
    const std::deque<std::string>& History() const { return history_; }
    void SetMaxHistory(size_t max) { maxHistory_ = max; }

    // --- Aliases ---
    void SetAlias(const std::string& name, const std::string& expansion);
    void RemoveAlias(const std::string& name);
    const std::unordered_map<std::string, std::string>& Aliases() const { return aliases_; }

    // --- Output ---
    void SetOutputFormat(ConsoleOutputFormat fmt) { outputFormat_ = fmt; }
    ConsoleOutputFormat GetOutputFormat() const { return outputFormat_; }

    /// Set a callback to receive output (for UI/socket frontends).
    void SetOutputCallback(OutputCallback cb) { outputCb_ = std::move(cb); }

    /// Format a ConsoleResult according to the current output format.
    std::string FormatResult(const ConsoleResult& result) const;

private:
    /// Tokenize input line into command + args, respecting quoted strings.
    static std::vector<std::string> Tokenize(const std::string& input);

    /// Resolve aliases (one level, no recursion to prevent infinite loops).
    std::string ResolveAlias(const std::string& command) const;

    KoiloKernel&     kernel_;
    CommandRegistry& registry_;

    std::deque<std::string> history_;
    size_t maxHistory_ = 256;

    std::unordered_map<std::string, std::string> aliases_;
    ConsoleOutputFormat outputFormat_ = ConsoleOutputFormat::Text;
    OutputCallback outputCb_;

    KL_BEGIN_FIELDS(ConsoleSession)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(ConsoleSession)
        KL_METHOD_AUTO(ConsoleSession, SetMaxHistory, "Set max history"),
        KL_METHOD_AUTO(ConsoleSession, SetAlias, "Set alias"),
        KL_METHOD_AUTO(ConsoleSession, RemoveAlias, "Remove alias"),
        KL_METHOD_AUTO(ConsoleSession, Aliases, "Aliases"),
        KL_METHOD_AUTO(ConsoleSession, SetOutputFormat, "Set output format"),
        KL_METHOD_AUTO(ConsoleSession, GetOutputFormat, "Get output format"),
        KL_METHOD_AUTO(ConsoleSession, FormatResult, "Format result")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(ConsoleSession)
        /* Requires references - no reflected ctors. */
    KL_END_DESCRIBE(ConsoleSession)

};

} // namespace koilo
