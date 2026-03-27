#include <koilo/kernel/console/console_session.hpp>
#include <koilo/kernel/console/console_commands.hpp>
#include <koilo/kernel/cvar/cvar_system.hpp>
#include <koilo/kernel/kernel.hpp>
#include <sstream>

namespace koilo {

ConsoleSession::ConsoleSession(KoiloKernel& kernel, CommandRegistry& registry)
    : kernel_(kernel), registry_(registry) {
}

ConsoleResult ConsoleSession::Execute(const std::string& input) {
    if (input.empty()) return ConsoleResult::Ok("");

    // Add to history
    if (history_.empty() || history_.back() != input) {
        history_.push_back(input);
        if (history_.size() > maxHistory_) {
            history_.pop_front();
        }
    }

    // Support pipe-separated commands (cmd1 | cmd2 not implemented yet, just note it)
    auto tokens = Tokenize(input);
    if (tokens.empty()) return ConsoleResult::Ok("");

    // Resolve alias on the command name
    std::string command = ResolveAlias(tokens[0]);

    // Re-tokenize if alias expanded to multiple words
    if (command != tokens[0] && command.find(' ') != std::string::npos) {
        auto expanded = Tokenize(command);
        for (size_t i = 1; i < tokens.size(); ++i) {
            expanded.push_back(tokens[i]);
        }
        tokens = std::move(expanded);
        command = tokens[0];
    }

    std::vector<std::string> args(tokens.begin() + 1, tokens.end());

    auto result = registry_.Execute(kernel_, command, args);

    // If command not found, try CVar fallback (typing cvar name gets/sets it)
    if (result.status == ConsoleResult::Status::NotFound) {
        auto cvarResult = TryCVarFallback(command, args);
        if (cvarResult.status != ConsoleResult::Status::Error || !cvarResult.text.empty())
            result = cvarResult;
    }

    // Deliver output via callback if set
    if (outputCb_) {
        outputCb_(FormatResult(result));
    }

    return result;
}

std::vector<std::string> ConsoleSession::Complete(const std::string& input) const {
    auto tokens = Tokenize(input);

    if (tokens.empty()) {
        // Complete command names
        return registry_.ListCommands();
    }

    bool endsWithSpace = !input.empty() && input.back() == ' ';

    if (tokens.size() == 1 && !endsWithSpace) {
        // Completing command name - also include CVar names
        auto matches = registry_.Complete(kernel_, "", {}, tokens[0]);
        CVarSystem::Get().ForEach([&](const CVarParameter& p) {
            if (p.name.compare(0, tokens[0].size(), tokens[0]) == 0)
                matches.push_back(p.name);
        });
        return matches;
    }

    // Completing arguments
    std::string partial = endsWithSpace ? "" : tokens.back();
    std::vector<std::string> args;
    size_t argEnd = endsWithSpace ? tokens.size() : tokens.size() - 1;
    for (size_t i = 1; i < argEnd; ++i) {
        args.push_back(tokens[i]);
    }
    return registry_.Complete(kernel_, tokens[0], args, partial);
}

void ConsoleSession::SetAlias(const std::string& name, const std::string& expansion) {
    aliases_[name] = expansion;
}

void ConsoleSession::RemoveAlias(const std::string& name) {
    aliases_.erase(name);
}

std::string ConsoleSession::FormatResult(const ConsoleResult& result) const {
    if (outputFormat_ == ConsoleOutputFormat::Json && !result.json.empty()) {
        return result.json;
    }
    return result.text;
}

std::string ConsoleSession::ResolveAlias(const std::string& command) const {
    // Check session-local aliases first, then global registry aliases
    auto it = aliases_.find(command);
    if (it != aliases_.end()) return it->second;
    return registry_.ResolveAlias(command);
}

std::vector<std::string> ConsoleSession::Tokenize(const std::string& input) {
    std::vector<std::string> tokens;
    std::string current;
    bool inQuote = false;
    char quoteChar = 0;

    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];

        if (inQuote) {
            if (c == quoteChar) {
                inQuote = false;
            } else {
                current += c;
            }
        } else if (c == '"' || c == '\'') {
            inQuote = true;
            quoteChar = c;
        } else if (c == ' ' || c == '\t') {
            if (!current.empty()) {
                tokens.push_back(std::move(current));
                current.clear();
            }
        } else {
            current += c;
        }
    }

    if (!current.empty()) {
        tokens.push_back(std::move(current));
    }

    return tokens;
}

} // namespace koilo
