// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/kernel/console/console_commands.hpp>
#include <koilo/kernel/config/config_store.hpp>
#include <koilo/kernel/kernel.hpp>
#include <sstream>

namespace koilo {

void RegisterConfigCommands(CommandRegistry& registry) {

    // -- config.list [prefix] ----------------------------------------
    registry.Register({"config.list",
        "config.list [prefix]",
        "List all config keys (optionally filtered by prefix)",
        [](KoiloKernel& kernel, const std::vector<std::string>& args) -> ConsoleResult {
            auto& cfg = kernel.GetConfig();
            const char* prefix = args.empty() ? "" : args[0].c_str();
            auto keys = cfg.Keys(prefix);
            if (keys.empty())
                return ConsoleResult::Ok("(no config entries)");

            std::ostringstream os;
            os << "Config entries";
            if (prefix[0]) os << " [" << prefix << "*]";
            os << " (" << keys.size() << "):\n";
            for (auto& k : keys) {
                os << "  " << k << " = " << cfg.GetString(k.c_str()) << '\n';
            }
            return ConsoleResult::Ok(os.str());
        },
        // Tab-complete: suggest known prefixes
        [](KoiloKernel& kernel, const std::vector<std::string>&, const std::string& partial) -> std::vector<std::string> {
            auto keys = kernel.GetConfig().Keys();
            // Extract unique namespace prefixes
            std::vector<std::string> completions;
            for (auto& k : keys) {
                if (k.compare(0, partial.size(), partial) == 0)
                    completions.push_back(k);
            }
            return completions;
        }
    });

    // -- config.get <key> --------------------------------------------
    registry.Register({"config.get",
        "config.get <key>",
        "Get a config value",
        [](KoiloKernel& kernel, const std::vector<std::string>& args) -> ConsoleResult {
            if (args.empty())
                return ConsoleResult::Error("Usage: config.get <key>");
            auto& cfg = kernel.GetConfig();
            if (!cfg.Has(args[0].c_str()))
                return ConsoleResult::Error("Key not found: " + args[0]);
            return ConsoleResult::Ok(args[0] + " = " + cfg.GetString(args[0].c_str()));
        },
        [](KoiloKernel& kernel, const std::vector<std::string>&, const std::string& partial) -> std::vector<std::string> {
            auto keys = kernel.GetConfig().Keys();
            std::vector<std::string> out;
            for (auto& k : keys)
                if (k.compare(0, partial.size(), partial) == 0) out.push_back(k);
            return out;
        }
    });

    // -- config.set <key> <value> ------------------------------------
    registry.Register({"config.set",
        "config.set <key> <value>",
        "Set a config value (auto-detects type: bool, int, float, or string)",
        [](KoiloKernel& kernel, const std::vector<std::string>& args) -> ConsoleResult {
            if (args.size() < 2)
                return ConsoleResult::Error("Usage: config.set <key> <value>");
            auto& cfg = kernel.GetConfig();
            const auto& key = args[0];
            const auto& val = args[1];

            // Auto-detect type
            if (val == "true" || val == "false") {
                cfg.SetBool(key.c_str(), val == "true");
            } else {
                // Try int
                bool isInt = true;
                for (size_t i = (val[0] == '-' ? 1 : 0); i < val.size(); ++i)
                    if (!std::isdigit(static_cast<unsigned char>(val[i]))) { isInt = false; break; }
                if (isInt && !val.empty()) {
                    cfg.SetInt(key.c_str(), std::stoi(val));
                } else {
                    // Try float
                    try {
                        size_t pos = 0;
                        float f = std::stof(val, &pos);
                        if (pos == val.size()) {
                            cfg.SetFloat(key.c_str(), f);
                        } else {
                            cfg.SetString(key.c_str(), val.c_str());
                        }
                    } catch (...) {
                        cfg.SetString(key.c_str(), val.c_str());
                    }
                }
            }
            return ConsoleResult::Ok(key + " = " + cfg.GetString(key.c_str()));
        },
        [](KoiloKernel& kernel, const std::vector<std::string>&, const std::string& partial) -> std::vector<std::string> {
            auto keys = kernel.GetConfig().Keys();
            std::vector<std::string> out;
            for (auto& k : keys)
                if (k.compare(0, partial.size(), partial) == 0) out.push_back(k);
            return out;
        }
    });

    // -- config.save [path] ------------------------------------------
    registry.Register({"config.save",
        "config.save [path]",
        "Save config to file (default: koilo.cfg)",
        [](KoiloKernel& kernel, const std::vector<std::string>& args) -> ConsoleResult {
            const char* path = args.empty() ? "koilo.cfg" : args[0].c_str();
            auto& cfg = kernel.GetConfig();
            cfg.PullArchiveCVars();
            if (!cfg.SaveToFile(path))
                return ConsoleResult::Error(std::string("Failed to save: ") + path);
            char buf[128];
            std::snprintf(buf, sizeof(buf), "Saved %zu entries to %s", cfg.Count(), path);
            return ConsoleResult::Ok(buf);
        }, nullptr
    });

    // -- config.load [path] ------------------------------------------
    registry.Register({"config.load",
        "config.load [path]",
        "Load config from file (default: koilo.cfg)",
        [](KoiloKernel& kernel, const std::vector<std::string>& args) -> ConsoleResult {
            const char* path = args.empty() ? "koilo.cfg" : args[0].c_str();
            auto& cfg = kernel.GetConfig();
            if (!cfg.LoadFromFile(path))
                return ConsoleResult::Error(std::string("Failed to load: ") + path);
            cfg.PushArchiveCVars();
            char buf[128];
            std::snprintf(buf, sizeof(buf), "Loaded %zu entries from %s", cfg.Count(), path);
            return ConsoleResult::Ok(buf);
        }, nullptr
    });

    // -- config.remove <key> -----------------------------------------
    registry.Register({"config.remove",
        "config.remove <key>",
        "Remove a config entry",
        [](KoiloKernel& kernel, const std::vector<std::string>& args) -> ConsoleResult {
            if (args.empty())
                return ConsoleResult::Error("Usage: config.remove <key>");
            auto& cfg = kernel.GetConfig();
            if (!cfg.Has(args[0].c_str()))
                return ConsoleResult::Error("Key not found: " + args[0]);
            cfg.Remove(args[0].c_str());
            return ConsoleResult::Ok("Removed: " + args[0]);
        },
        [](KoiloKernel& kernel, const std::vector<std::string>&, const std::string& partial) -> std::vector<std::string> {
            auto keys = kernel.GetConfig().Keys();
            std::vector<std::string> out;
            for (auto& k : keys)
                if (k.compare(0, partial.size(), partial) == 0) out.push_back(k);
            return out;
        }
    });
}

} // namespace koilo
