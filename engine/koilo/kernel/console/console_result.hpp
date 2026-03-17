// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file console_result.hpp
 * @brief Structured result type returned by console commands.
 *
 * @date 12/06/2025
 * @author Coela
 */
#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "../../registry/reflect_macros.hpp"

namespace koilo {

/// Structured result from a console command.
struct ConsoleResult {
    enum class Status { Ok, Error, NotFound, Denied };

    Status      status = Status::Ok;
    std::string text;    // human-readable output
    std::string json;    // structured data (optional)

    static ConsoleResult Ok(const std::string& text, const std::string& json = "") {
        return {Status::Ok, text, json};
    }
    static ConsoleResult Error(const std::string& text) {
        return {Status::Error, text, ""};
    }
    static ConsoleResult NotFound(const std::string& text) {
        return {Status::NotFound, text, ""};
    }
    static ConsoleResult Denied(const std::string& text) {
        return {Status::Denied, text, ""};
    }

    bool ok() const { return status == Status::Ok; }

    KL_BEGIN_FIELDS(ConsoleResult)
        KL_FIELD(ConsoleResult, status, "Status", 0, 0),
        KL_FIELD(ConsoleResult, text, "Text", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(ConsoleResult)
        KL_SMETHOD_AUTO(ConsoleResult::Error, "Error"),
        KL_SMETHOD_AUTO(ConsoleResult::NotFound, "Not found"),
        KL_SMETHOD_AUTO(ConsoleResult::Denied, "Denied"),
        KL_METHOD_AUTO(ConsoleResult, ok, "Ok")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(ConsoleResult)
        /* No reflected ctors. */
    KL_END_DESCRIBE(ConsoleResult)

};

/// Output format preference.
enum class ConsoleOutputFormat { Text, Json };

} // namespace koilo
