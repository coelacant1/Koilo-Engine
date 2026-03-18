// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file config_provider.hpp
 * @brief Abstract interface for hierarchical, typed configuration access.
 *
 * Keys use dot-separated namespaces: "display.width", "render.vsync".
 * Implementations may back onto files, in-memory maps, or remote stores.
 *
 * @date 01/15/2026
 * @author Coela
 */
#pragma once

namespace koilo {

class IConfigProvider {
public:
    virtual ~IConfigProvider() = default;

    // --- Typed getters (return fallback when key absent) ---

    virtual bool        GetBool  (const char* key, bool        fallback = false) const = 0;
    virtual int         GetInt   (const char* key, int         fallback = 0)     const = 0;
    virtual float       GetFloat (const char* key, float       fallback = 0.0f)  const = 0;
    virtual const char* GetString(const char* key, const char* fallback = "")    const = 0;

    // --- Setters ---

    virtual void SetBool  (const char* key, bool value)        = 0;
    virtual void SetInt   (const char* key, int value)         = 0;
    virtual void SetFloat (const char* key, float value)       = 0;
    virtual void SetString(const char* key, const char* value) = 0;

    // --- Queries ---

    virtual bool Has   (const char* key) const = 0;
    virtual void Remove(const char* key)       = 0;

    // --- Persistence ---

    virtual bool SaveToFile (const char* path) const = 0;
    virtual bool LoadFromFile(const char* path)      = 0;
};

} // namespace koilo
