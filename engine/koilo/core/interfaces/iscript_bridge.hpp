// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <string>

namespace koilo {

/// Minimal interface for script engine access from the kernel layer.
/// Implemented by KoiloScriptEngine. Used by ModuleLoader to avoid
/// a direct dependency on the scripting subsystem.
class IScriptBridge {
public:
    virtual ~IScriptBridge() = default;

    /// Register a C++ object as a script-accessible global.
    virtual void RegisterGlobal(const char* name, const char* className, void* instance) = 0;

    /// Set a script variable by name (numeric value).
    virtual void SetScriptVariable(const std::string& name, double value) = 0;

    /// Get a script variable by name (numeric value, 0.0 if not found).
    virtual double GetScriptVariableNum(const std::string& name) const = 0;
};

} // namespace koilo
