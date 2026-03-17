#include <koilo/kernel/console/command_registry.hpp>
#include <algorithm>

namespace koilo {

void CommandRegistry::Register(CommandDef def) {
    std::string name = def.name;
    commands_[name] = std::move(def);
}

ConsoleResult CommandRegistry::Execute(KoiloKernel& kernel, const std::string& name,
                                        const std::vector<std::string>& args) const {
    auto it = commands_.find(name);
    if (it == commands_.end()) {
        return ConsoleResult::NotFound("Unknown command: " + name);
    }
    return it->second.handler(kernel, args);
}

std::vector<std::string> CommandRegistry::Complete(KoiloKernel& kernel, const std::string& name,
                                                    const std::vector<std::string>& args,
                                                    const std::string& partial) const {
    auto it = commands_.find(name);
    if (it != commands_.end() && it->second.completer) {
        return it->second.completer(kernel, args, partial);
    }

    // Default: complete command names
    std::vector<std::string> matches;
    for (auto& [cmdName, _] : commands_) {
        if (cmdName.find(partial) == 0) {
            matches.push_back(cmdName);
        }
    }
    std::sort(matches.begin(), matches.end());
    return matches;
}

const CommandDef* CommandRegistry::Find(const std::string& name) const {
    auto it = commands_.find(name);
    return it != commands_.end() ? &it->second : nullptr;
}

std::vector<std::string> CommandRegistry::ListCommands() const {
    std::vector<std::string> names;
    names.reserve(commands_.size());
    for (auto& [name, _] : commands_) {
        names.push_back(name);
    }
    std::sort(names.begin(), names.end());
    return names;
}

} // namespace koilo
