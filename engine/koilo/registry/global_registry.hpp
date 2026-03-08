// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include "registry.hpp"

namespace koilo {

// Fast name->ClassDesc lookup (built lazily from the registration vector)
inline std::unordered_map<std::string, const ClassDesc*>& ClassRegistryMap() {
    static std::unordered_map<std::string, const ClassDesc*> m;
    return m;
}

// Ordered list kept for iteration / backward compat
inline std::vector<const ClassDesc*>& GlobalClassRegistry() {
    static std::vector<const ClassDesc*> v;
    return v;
}

struct AutoRegistrar {
    explicit AutoRegistrar(const ClassDesc* cd) {
        GlobalClassRegistry().push_back(cd);
        if (cd && cd->name) ClassRegistryMap()[cd->name] = cd;
    }
};

// Per-class field/method hash caches (built lazily on first lookup)
inline std::unordered_map<const ClassDesc*, std::unordered_map<std::string, const FieldDecl*>>& FieldCache() {
    static std::unordered_map<const ClassDesc*, std::unordered_map<std::string, const FieldDecl*>> c;
    return c;
}
inline std::unordered_map<const ClassDesc*, std::unordered_map<std::string, std::vector<const MethodDesc*>>>& MethodCache() {
    static std::unordered_map<const ClassDesc*, std::unordered_map<std::string, std::vector<const MethodDesc*>>> c;
    return c;
}

}
