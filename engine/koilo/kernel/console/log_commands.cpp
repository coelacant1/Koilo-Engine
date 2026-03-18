// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file log_commands.cpp
 * @brief Console commands for the structured logging system.
 *
 * Commands: log.level, log.channels, log.mute, log.unmute, log.tail,
 *           log.file, log.clear, log.filter
 *
 * @date 12/02/2025
 * @author Coela Can't
 */

#include "console_commands.hpp"
#include "../logging/log_system.hpp"
#include <cstring>
#include <string>
#include <sstream>
#include <algorithm>

namespace koilo {

static LogLevel ParseLevel(const std::string& s) {
    if (s == "error" || s == "err")     return LogLevel::Error;
    if (s == "warn"  || s == "warning") return LogLevel::Warn;
    if (s == "info")                    return LogLevel::Info;
    if (s == "debug" || s == "dbg")     return LogLevel::Debug;
    if (s == "trace")                   return LogLevel::Trace;
    return LogLevel::Info;
}

static std::string ToLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

void RegisterLogCommands(CommandRegistry& registry) {
    // log.level [level] | log.level <channel> <level>
    registry.Register({"log.level", "log.level [level|channel level]",
        "Get/set global or per-channel log level",
        [](KoiloKernel&, const std::vector<std::string>& args) -> ConsoleResult {
            auto* ls = GetLogSystem();
            if (!ls) return ConsoleResult::Error("No log system active");

            if (args.empty()) {
                return ConsoleResult::Ok(
                    std::string("Global log level: ") + LogLevelStr(ls->GetLevel()));
            }

            if (args.size() >= 2) {
                std::string channel = args[0];
                std::string level = ToLower(args[1]);
                ls->SetChannelLevel(channel, ParseLevel(level));
                return ConsoleResult::Ok("Channel '" + channel + "' level set to " + level);
            }

            std::string level = ToLower(args[0]);
            ls->SetLevel(ParseLevel(level));
            return ConsoleResult::Ok("Global log level set to " + level);
        },
        // Completer: suggest level names
        [](KoiloKernel&, const std::vector<std::string>&,
           const std::string& partial) -> std::vector<std::string> {
            std::vector<std::string> levels = {"error", "warn", "info", "debug", "trace"};
            std::vector<std::string> matches;
            for (auto& l : levels)
                if (l.find(partial) == 0) matches.push_back(l);
            return matches;
        }
    });

    // log.channels
    registry.Register({"log.channels", "log.channels",
        "List all channels that have logged",
        [](KoiloKernel&, const std::vector<std::string>&) -> ConsoleResult {
            auto* ls = GetLogSystem();
            if (!ls) return ConsoleResult::Error("No log system active");
            auto channels = ls->ListChannels();
            if (channels.empty()) return ConsoleResult::Ok("No channels have logged yet");
            std::ostringstream os;
            os << channels.size() << " channels:\n";
            for (auto& ch : channels) {
                os << "  " << ch;
                if (ls->IsChannelMuted(ch)) os << " (muted)";
                os << "\n";
            }
            return ConsoleResult::Ok(os.str());
        }, nullptr
    });

    // log.mute <channel>
    registry.Register({"log.mute", "log.mute <channel>",
        "Mute a log channel",
        [](KoiloKernel&, const std::vector<std::string>& args) -> ConsoleResult {
            auto* ls = GetLogSystem();
            if (!ls) return ConsoleResult::Error("No log system active");
            if (args.empty()) return ConsoleResult::Error("Usage: log.mute <channel>");
            ls->MuteChannel(args[0]);
            return ConsoleResult::Ok("Muted channel: " + args[0]);
        },
        // Completer: channel names
        [](KoiloKernel&, const std::vector<std::string>&,
           const std::string& partial) -> std::vector<std::string> {
            auto* ls = GetLogSystem();
            if (!ls) return {};
            std::vector<std::string> matches;
            for (auto& ch : ls->ListChannels())
                if (ch.find(partial) == 0) matches.push_back(ch);
            return matches;
        }
    });

    // log.unmute <channel>
    registry.Register({"log.unmute", "log.unmute <channel>",
        "Unmute a log channel",
        [](KoiloKernel&, const std::vector<std::string>& args) -> ConsoleResult {
            auto* ls = GetLogSystem();
            if (!ls) return ConsoleResult::Error("No log system active");
            if (args.empty()) return ConsoleResult::Error("Usage: log.unmute <channel>");
            ls->UnmuteChannel(args[0]);
            return ConsoleResult::Ok("Unmuted channel: " + args[0]);
        },
        [](KoiloKernel&, const std::vector<std::string>&,
           const std::string& partial) -> std::vector<std::string> {
            auto* ls = GetLogSystem();
            if (!ls) return {};
            std::vector<std::string> matches;
            for (auto& ch : ls->ListChannels())
                if (ch.find(partial) == 0) matches.push_back(ch);
            return matches;
        }
    });

    // log.tail [N] [channel]
    registry.Register({"log.tail", "log.tail [N] [channel]",
        "Show last N log entries (default 20)",
        [](KoiloKernel&, const std::vector<std::string>& args) -> ConsoleResult {
            auto* ls = GetLogSystem();
            if (!ls) return ConsoleResult::Error("No log system active");

            size_t count = 20;
            std::string filterChannel;

            for (auto& arg : args) {
                char* end = nullptr;
                long n = strtol(arg.c_str(), &end, 10);
                if (end != arg.c_str() && *end == '\0' && n > 0)
                    count = static_cast<size_t>(n);
                else
                    filterChannel = arg;
            }

            size_t total = ls->HistoryCount();
            if (total == 0) return ConsoleResult::Ok("(no log entries)");

            std::vector<size_t> matches;
            for (size_t i = total; i-- > 0 && matches.size() < count; ) {
                const LogRecord* rec = ls->HistoryAt(i);
                if (!rec) continue;
                if (!filterChannel.empty() && filterChannel != rec->channel)
                    continue;
                matches.push_back(i);
            }

            std::ostringstream os;
            char buf[640];
            for (auto it = matches.rbegin(); it != matches.rend(); ++it) {
                const LogRecord* rec = ls->HistoryAt(*it);
                if (!rec) continue;
                double secs = static_cast<double>(rec->timestamp) / 1000.0;
                snprintf(buf, sizeof(buf), "[%7.2fs] [%-5s] [%s] %s",
                         secs, LogLevelStr(rec->level), rec->channel, rec->message);
                os << buf << "\n";
            }
            return ConsoleResult::Ok(os.str());
        }, nullptr
    });

    // log.file [path]
    registry.Register({"log.file", "log.file [path]",
        "Enable file logging (no args = disable)",
        [](KoiloKernel&, const std::vector<std::string>& args) -> ConsoleResult {
            auto* ls = GetLogSystem();
            if (!ls) return ConsoleResult::Error("No log system active");
            if (args.empty()) {
                ls->CloseLogFile();
                return ConsoleResult::Ok("File logging disabled");
            }
            ls->OpenLogFile(args[0]);
            return ConsoleResult::Ok("Logging to file: " + args[0]);
        }, nullptr
    });

    // log.clear
    registry.Register({"log.clear", "log.clear",
        "Clear the log history buffer",
        [](KoiloKernel&, const std::vector<std::string>&) -> ConsoleResult {
            auto* ls = GetLogSystem();
            if (!ls) return ConsoleResult::Error("No log system active");
            ls->ClearHistory();
            return ConsoleResult::Ok("Log history cleared");
        }, nullptr
    });

    // log.filter <substring>
    registry.Register({"log.filter", "log.filter <substring>",
        "Search log history for matching entries",
        [](KoiloKernel&, const std::vector<std::string>& args) -> ConsoleResult {
            auto* ls = GetLogSystem();
            if (!ls) return ConsoleResult::Error("No log system active");
            if (args.empty()) return ConsoleResult::Error("Usage: log.filter <substring>");

            std::string needle = ToLower(args[0]);

            size_t total = ls->HistoryCount();
            size_t shown = 0;
            std::ostringstream os;
            char buf[640];
            for (size_t i = 0; i < total && shown < 50; ++i) {
                const LogRecord* rec = ls->HistoryAt(i);
                if (!rec) continue;
                std::string msg = ToLower(rec->message);
                std::string ch = ToLower(rec->channel);
                if (msg.find(needle) == std::string::npos &&
                    ch.find(needle) == std::string::npos)
                    continue;
                double secs = static_cast<double>(rec->timestamp) / 1000.0;
                snprintf(buf, sizeof(buf), "[%7.2fs] [%-5s] [%s] %s",
                         secs, LogLevelStr(rec->level), rec->channel, rec->message);
                os << buf << "\n";
                ++shown;
            }
            os << "(" << shown << " matches, " << total << " total entries)";
            return ConsoleResult::Ok(os.str());
        }, nullptr
    });
}

} // namespace koilo
