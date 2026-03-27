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

    // Default: complete command names (case-insensitive prefix match)
    std::vector<std::string> matches;
    std::string lowerPartial = partial;
    std::transform(lowerPartial.begin(), lowerPartial.end(),
                   lowerPartial.begin(), ::tolower);
    for (auto& [cmdName, _] : commands_) {
        std::string lowerCmd = cmdName;
        std::transform(lowerCmd.begin(), lowerCmd.end(),
                       lowerCmd.begin(), ::tolower);
        if (lowerCmd.find(lowerPartial) == 0) {
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

void CommandRegistry::SetAlias(const std::string& name, const std::string& expansion) {
    aliases_[name] = expansion;
}

void CommandRegistry::RemoveAlias(const std::string& name) {
    aliases_.erase(name);
}

std::string CommandRegistry::ResolveAlias(const std::string& name) const {
    auto it = aliases_.find(name);
    return it != aliases_.end() ? it->second : name;
}

} // namespace koilo
