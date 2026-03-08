// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file script_class.cpp
 * @brief Implementation of ScriptInstance field/method access.
 *
 * @date 23/02/2026
 * @version 1.0
 * @author Coela
 */

#include <koilo/scripting/script_class.hpp>
#include <koilo/scripting/script_context.hpp>

namespace koilo {
namespace scripting {

Value ScriptInstance::GetField(const std::string& name) const {
    if (klass) {
        int idx = klass->FindField(name);
        if (idx >= 0 && idx < static_cast<int>(fields.size())) {
            return fields[idx];
        }
    }
    auto it = extraFields.find(name);
    if (it != extraFields.end()) return it->second;
    return Value();
}

void ScriptInstance::SetField(const std::string& name, const Value& val) {
    if (klass) {
        int idx = klass->FindField(name);
        if (idx >= 0 && idx < static_cast<int>(fields.size())) {
            fields[idx] = val;
            return;
        }
    }
    extraFields[name] = val;
}

bool ScriptInstance::HasField(const std::string& name) const {
    if (klass && klass->FindField(name) >= 0) return true;
    return extraFields.count(name) > 0;
}

bool ScriptInstance::HasMethod(const std::string& name) const {
    return klass && klass->HasMethod(name);
}

} // namespace scripting
} // namespace koilo
