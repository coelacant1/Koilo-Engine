// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file cvar_system.cpp
 * @brief CVar system implementation.
 *
 * @date 11/04/2025
 * @author Coela
 */
#include <koilo/kernel/cvar/cvar_system.hpp>
#include <koilo/kernel/logging/log.hpp>

#include <algorithm>
#include <cassert>
#include <map>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace koilo {

// -- CVarParameter helpers --------------------------------------------

std::string CVarParameter::ValueString() const {
    switch (type) {
        case CVarType::Int:    return std::to_string(intVal);
        case CVarType::Float:  return std::to_string(floatVal);
        case CVarType::Bool:   return boolVal ? "1" : "0";
        case CVarType::String: return strVal;
    }
    return {};
}

std::string CVarParameter::DefaultString() const {
    switch (type) {
        case CVarType::Int:    return std::to_string(intDefault);
        case CVarType::Float:  return std::to_string(floatDefault);
        case CVarType::Bool:   return boolDefault ? "1" : "0";
        case CVarType::String: return strDefault;
    }
    return {};
}

bool CVarParameter::SetFromString(const std::string& val) {
    if (HasFlag(flags, CVarFlags::ReadOnly)) return false;
    switch (type) {
        case CVarType::Int: {
            try {
                int32_t v = static_cast<int32_t>(std::stol(val));
                int32_t old = intVal;
                intVal = v;
                if (onIntChanged && old != v) onIntChanged(old, v);
            } catch (...) { return false; }
            return true;
        }
        case CVarType::Float: {
            try {
                float v = std::stof(val);
                float old = floatVal;
                floatVal = v;
                if (onFloatChanged && old != v) onFloatChanged(old, v);
            } catch (...) { return false; }
            return true;
        }
        case CVarType::Bool: {
            bool v = (val == "1" || val == "true" || val == "on" || val == "yes");
            bool old = boolVal;
            boolVal = v;
            if (onBoolChanged && old != v) onBoolChanged(old, v);
            return true;
        }
        case CVarType::String: {
            std::string old = strVal;
            strVal = val;
            if (onStrChanged && old != val) onStrChanged(old, val);
            return true;
        }
    }
    return false;
}

void CVarParameter::ResetToDefault() {
    switch (type) {
        case CVarType::Int: {
            int32_t old = intVal;
            intVal = intDefault;
            if (onIntChanged && old != intDefault) onIntChanged(old, intDefault);
            break;
        }
        case CVarType::Float: {
            float old = floatVal;
            floatVal = floatDefault;
            if (onFloatChanged && old != floatDefault) onFloatChanged(old, floatDefault);
            break;
        }
        case CVarType::Bool: {
            bool old = boolVal;
            boolVal = boolDefault;
            if (onBoolChanged && old != boolDefault) onBoolChanged(old, boolDefault);
            break;
        }
        case CVarType::String: {
            std::string old = strVal;
            strVal = strDefault;
            if (onStrChanged && old != strDefault) onStrChanged(old, strDefault);
            break;
        }
    }
}

// -- CVarSystem internals ---------------------------------------------

struct CVarSystem::Impl {
    std::mutex                                          mu;
    std::map<std::string, std::unique_ptr<CVarParameter>> params;  // sorted by name
    std::unordered_map<std::string, CVarParameter*>     lookup;    // fast O(1) lookup
};

CVarSystem& CVarSystem::Get() {
    static CVarSystem instance;
    return instance;
}

CVarSystem::Impl& CVarSystem::GetImpl() {
    static Impl impl;
    return impl;
}

const CVarSystem::Impl& CVarSystem::GetImpl() const {
    // const overload delegates to the same static
    return const_cast<CVarSystem*>(this)->GetImpl();
}

CVarParameter* CVarSystem::Register(const std::string& name,
                                     const std::string& description,
                                     CVarType type,
                                     CVarFlags flags) {
    auto& impl = GetImpl();
    std::lock_guard<std::mutex> lock(impl.mu);

    auto it = impl.lookup.find(name);
    if (it != impl.lookup.end()) {
        KL_WARN("CVar", "'%s' registered twice, keeping first", name.c_str());
        return it->second;
    }

    auto param = std::make_unique<CVarParameter>();
    param->name        = name;
    param->description = description;
    param->type        = type;
    param->flags       = flags;

    CVarParameter* raw = param.get();
    impl.params[name] = std::move(param);
    impl.lookup[name] = raw;
    return raw;
}

CVarParameter* CVarSystem::Find(const std::string& name) {
    auto& impl = GetImpl();
    auto it = impl.lookup.find(name);
    return it != impl.lookup.end() ? it->second : nullptr;
}

const CVarParameter* CVarSystem::Find(const std::string& name) const {
    auto& impl = GetImpl();
    auto it = impl.lookup.find(name);
    return it != impl.lookup.end() ? it->second : nullptr;
}

void CVarSystem::ForEach(const std::function<void(const CVarParameter&)>& fn) const {
    auto& impl = GetImpl();
    for (auto& [name, param] : impl.params)
        fn(*param);
}

std::vector<const CVarParameter*> CVarSystem::WithPrefix(const std::string& prefix) const {
    std::vector<const CVarParameter*> result;
    auto& impl = GetImpl();
    // Use sorted map lower_bound for efficient prefix scan
    for (auto it = impl.params.lower_bound(prefix); it != impl.params.end(); ++it) {
        if (it->first.compare(0, prefix.size(), prefix) != 0) break;
        result.push_back(it->second.get());
    }
    return result;
}

size_t CVarSystem::Count() const {
    return GetImpl().params.size();
}

// -- D3: typed handle resolvers ---------------------------------------

CVarSystem::Handle<int32_t> CVarSystem::GetIntHandle(const std::string& name) {
    return Handle<int32_t>(Find(name));
}
CVarSystem::Handle<float> CVarSystem::GetFloatHandle(const std::string& name) {
    return Handle<float>(Find(name));
}
CVarSystem::Handle<bool> CVarSystem::GetBoolHandle(const std::string& name) {
    return Handle<bool>(Find(name));
}
CVarSystem::Handle<std::string> CVarSystem::GetStringHandle(const std::string& name) {
    return Handle<std::string>(Find(name));
}

// -- AutoCVar_Int -----------------------------------------------------

AutoCVar_Int::AutoCVar_Int(const char* name, const char* desc, int32_t defaultVal,
                           CVarFlags flags) {
    param_ = CVarSystem::Get().Register(name, desc, CVarType::Int, flags);
    param_->intVal     = defaultVal;
    param_->intDefault = defaultVal;
}

void AutoCVar_Int::Set(int32_t val) {
    if (HasFlag(param_->flags, CVarFlags::ReadOnly)) return;
    int32_t old = param_->intVal;
    param_->intVal = val;
    if (param_->onIntChanged && old != val) param_->onIntChanged(old, val);
}

void AutoCVar_Int::OnChanged(std::function<void(int32_t, int32_t)> cb) {
    param_->onIntChanged = std::move(cb);
}

// -- AutoCVar_Float ---------------------------------------------------

AutoCVar_Float::AutoCVar_Float(const char* name, const char* desc, float defaultVal,
                               CVarFlags flags) {
    param_ = CVarSystem::Get().Register(name, desc, CVarType::Float, flags);
    param_->floatVal     = defaultVal;
    param_->floatDefault = defaultVal;
}

void AutoCVar_Float::Set(float val) {
    if (HasFlag(param_->flags, CVarFlags::ReadOnly)) return;
    float old = param_->floatVal;
    param_->floatVal = val;
    if (param_->onFloatChanged && old != val) param_->onFloatChanged(old, val);
}

void AutoCVar_Float::OnChanged(std::function<void(float, float)> cb) {
    param_->onFloatChanged = std::move(cb);
}

// -- AutoCVar_Bool ----------------------------------------------------

AutoCVar_Bool::AutoCVar_Bool(const char* name, const char* desc, bool defaultVal,
                             CVarFlags flags) {
    param_ = CVarSystem::Get().Register(name, desc, CVarType::Bool, flags);
    param_->boolVal     = defaultVal;
    param_->boolDefault = defaultVal;
}

void AutoCVar_Bool::Set(bool val) {
    if (HasFlag(param_->flags, CVarFlags::ReadOnly)) return;
    bool old = param_->boolVal;
    param_->boolVal = val;
    if (param_->onBoolChanged && old != val) param_->onBoolChanged(old, val);
}

void AutoCVar_Bool::OnChanged(std::function<void(bool, bool)> cb) {
    param_->onBoolChanged = std::move(cb);
}

// -- AutoCVar_String --------------------------------------------------

AutoCVar_String::AutoCVar_String(const char* name, const char* desc, const char* defaultVal,
                                 CVarFlags flags) {
    param_ = CVarSystem::Get().Register(name, desc, CVarType::String, flags);
    param_->strVal     = defaultVal;
    param_->strDefault = defaultVal;
}

void AutoCVar_String::Set(const std::string& val) {
    if (HasFlag(param_->flags, CVarFlags::ReadOnly)) return;
    std::string old = param_->strVal;
    param_->strVal = val;
    if (param_->onStrChanged && old != val) param_->onStrChanged(old, val);
}

void AutoCVar_String::OnChanged(std::function<void(const std::string&, const std::string&)> cb) {
    param_->onStrChanged = std::move(cb);
}

} // namespace koilo
