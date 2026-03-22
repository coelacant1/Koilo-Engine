// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file cvar_system.hpp
 * @brief Runtime console variable system with self-registering static CVars.
 *
 * CVars are named, typed variables that can be inspected and modified at
 * runtime through the console, TCP connection, or UI. Static AutoCVar
 * instances register themselves before main() - no manual setup needed.
 *
 * @date 11/04/2025
 * @author Coela
 */
#pragma once

#include <cstdint>
#include <string>
#include <functional>
#include <vector>
#include "../../registry/reflect_macros.hpp"

namespace koilo {

// -- Flags ------------------------------------------------------------

enum class CVarFlags : uint32_t {
    None     = 0,
    ReadOnly = 1 << 0,   // Cannot be changed at runtime
    Archive  = 1 << 1,   // Saved / loaded from config file
    Advanced = 1 << 2,   // Hidden from basic listings
    Cheat    = 1 << 3,   // Requires cheat mode to modify
};

inline CVarFlags  operator|(CVarFlags a, CVarFlags b)  { return static_cast<CVarFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b)); }
inline CVarFlags  operator&(CVarFlags a, CVarFlags b)  { return static_cast<CVarFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b)); }
inline CVarFlags& operator|=(CVarFlags& a, CVarFlags b){ a = a | b; return a; }
inline bool       HasFlag(CVarFlags flags, CVarFlags f) { return (flags & f) != CVarFlags::None; }

// -- Types ------------------------------------------------------------

enum class CVarType : uint8_t { Int, Float, Bool, String };

// -- CVarParameter ----------------------------------------------------

struct CVarParameter {
    std::string  name;
    std::string  description;
    CVarType     type     = CVarType::Int;
    CVarFlags    flags    = CVarFlags::None;

    // Current and default values (only the matching type field is valid).
    int32_t      intVal       = 0;
    int32_t      intDefault   = 0;
    float        floatVal     = 0.0f;
    float        floatDefault = 0.0f;
    bool         boolVal      = false;
    bool         boolDefault  = false;
    std::string  strVal;
    std::string  strDefault;

    // Change callbacks (optional).
    std::function<void(int32_t, int32_t)>              onIntChanged;
    std::function<void(float, float)>                  onFloatChanged;
    std::function<void(bool, bool)>                    onBoolChanged;
    std::function<void(const std::string&, const std::string&)> onStrChanged;

    /// Format current value as a string.
    std::string ValueString() const;

    /// Format default value as a string.
    std::string DefaultString() const;

    /// Set value from a string. Returns false if parse fails or ReadOnly.
    bool SetFromString(const std::string& val);

    /// Reset to default value.
    void ResetToDefault();
};

// -- CVarSystem -------------------------------------------------------

/// Global CVar registry. Thread-safe for registration (which happens
/// from static constructors before main). Reads are lock-free.
class CVarSystem {
public:
    static CVarSystem& Get();

    /// Register a new CVar. Called from AutoCVar constructors.
    /// Returns pointer to the stored parameter (stable for program lifetime).
    CVarParameter* Register(const std::string& name,
                            const std::string& description,
                            CVarType type,
                            CVarFlags flags);

    /// Look up a CVar by name. Returns nullptr if not found.
    CVarParameter* Find(const std::string& name);
    const CVarParameter* Find(const std::string& name) const;

    /// Iterate all CVars (alphabetical order).
    void ForEach(const std::function<void(const CVarParameter&)>& fn) const;

    /// Return all CVars whose name starts with the given prefix.
    std::vector<const CVarParameter*> WithPrefix(const std::string& prefix) const;

    /// Total registered CVar count.
    size_t Count() const;

private:
    CVarSystem() = default;
    struct Impl;
    Impl& GetImpl();
    const Impl& GetImpl() const;
};

// -- AutoCVar (self-registering static helpers) -----------------------

/// Integer CVar. Declare as a file-scope static:
///   static AutoCVar_Int cvar_maxFps("r.maxfps", "Frame rate cap", 60);
class AutoCVar_Int {
public:
    AutoCVar_Int(const char* name, const char* desc, int32_t defaultVal,
                 CVarFlags flags = CVarFlags::None);

    int32_t Get() const       { return param_->intVal; }
    void    Set(int32_t val);
    CVarParameter* GetParam() { return param_; }

    void OnChanged(std::function<void(int32_t, int32_t)> cb);

private:
    CVarParameter* param_;

    KL_BEGIN_FIELDS(AutoCVar_Int)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(AutoCVar_Int)
        KL_METHOD_AUTO(AutoCVar_Int, Get, "Get"),
        KL_METHOD_AUTO(AutoCVar_Int, Set, "Set"),
        KL_METHOD_AUTO(AutoCVar_Int, GetParam, "Get param"),
        KL_METHOD_AUTO(AutoCVar_Int, OnChanged, "On changed")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(AutoCVar_Int)
        KL_CTOR(AutoCVar_Int, const char*, const char*, int32_t, CVarFlags)
    KL_END_DESCRIBE(AutoCVar_Int)

};

/// Float CVar.
class AutoCVar_Float {
public:
    AutoCVar_Float(const char* name, const char* desc, float defaultVal,
                   CVarFlags flags = CVarFlags::None);

    float Get() const       { return param_->floatVal; }
    void  Set(float val);
    CVarParameter* GetParam() { return param_; }

    void OnChanged(std::function<void(float, float)> cb);

private:
    CVarParameter* param_;

    KL_BEGIN_FIELDS(AutoCVar_Float)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(AutoCVar_Float)
        KL_METHOD_AUTO(AutoCVar_Float, Get, "Get"),
        KL_METHOD_AUTO(AutoCVar_Float, Set, "Set"),
        KL_METHOD_AUTO(AutoCVar_Float, GetParam, "Get param"),
        KL_METHOD_AUTO(AutoCVar_Float, OnChanged, "On changed")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(AutoCVar_Float)
        KL_CTOR(AutoCVar_Float, const char*, const char*, float, CVarFlags)
    KL_END_DESCRIBE(AutoCVar_Float)

};

/// Bool CVar.
class AutoCVar_Bool {
public:
    AutoCVar_Bool(const char* name, const char* desc, bool defaultVal,
                  CVarFlags flags = CVarFlags::None);

    bool Get() const       { return param_->boolVal; }
    void Set(bool val);
    CVarParameter* GetParam() { return param_; }

    void OnChanged(std::function<void(bool, bool)> cb);

private:
    CVarParameter* param_;

    KL_BEGIN_FIELDS(AutoCVar_Bool)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(AutoCVar_Bool)
        KL_METHOD_AUTO(AutoCVar_Bool, Get, "Get"),
        KL_METHOD_AUTO(AutoCVar_Bool, Set, "Set"),
        KL_METHOD_AUTO(AutoCVar_Bool, GetParam, "Get param"),
        KL_METHOD_AUTO(AutoCVar_Bool, OnChanged, "On changed")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(AutoCVar_Bool)
        KL_CTOR(AutoCVar_Bool, const char*, const char*, bool, CVarFlags)
    KL_END_DESCRIBE(AutoCVar_Bool)

};

/// String CVar.
class AutoCVar_String {
public:
    AutoCVar_String(const char* name, const char* desc, const char* defaultVal,
                    CVarFlags flags = CVarFlags::None);

    const std::string& Get() const { return param_->strVal; }
    void Set(const std::string& val);
    CVarParameter* GetParam() { return param_; }

    void OnChanged(std::function<void(const std::string&, const std::string&)> cb);

private:
    CVarParameter* param_;

    KL_BEGIN_FIELDS(AutoCVar_String)
        /* No reflected fields. */
    KL_END_FIELDS

    KL_BEGIN_METHODS(AutoCVar_String)
        KL_METHOD_AUTO(AutoCVar_String, Get, "Get"),
        KL_METHOD_AUTO(AutoCVar_String, Set, "Set"),
        KL_METHOD_AUTO(AutoCVar_String, GetParam, "Get param"),
        KL_METHOD_AUTO(AutoCVar_String, OnChanged, "On changed")
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(AutoCVar_String)
        KL_CTOR(AutoCVar_String, const char*, const char*, const char*, CVarFlags)
    KL_END_DESCRIBE(AutoCVar_String)

};

} // namespace koilo
