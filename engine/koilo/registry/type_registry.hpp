// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "registry.hpp"

#if KL_HAS_RTTI
#include <typeindex>
#include <unordered_map>
#include <string>
#include <cstdint>

namespace koilo {

// TypeInfo describes a type's relationship to the reflection system.
struct TypeInfo {
    std::string name;               ///< Human-readable class name
    bool is_pointer = false;        ///< True if the type is a pointer (e.g., Light*)
    bool is_reference = false;      ///< True if the type is a reference (e.g., Light&)
    const ClassDesc* classDesc = nullptr; ///< ClassDesc for the pointed-to/referenced class (if known)
};

// Global type_index -> TypeInfo map, populated at Describe() time.
inline std::unordered_map<std::type_index, TypeInfo>& TypeRegistry() {
    static std::unordered_map<std::type_index, TypeInfo> reg;
    return reg;
}

// Register a concrete class type and its pointer/const-pointer variants.
template<typename T>
inline void RegisterTypeInfo(const ClassDesc* cd) {
    auto& reg = TypeRegistry();
    reg[std::type_index(typeid(T))] = { cd->name, false, false, cd };
    reg[std::type_index(typeid(T*))] = { std::string(cd->name) + "*", true, false, cd };
    reg[std::type_index(typeid(const T*))] = { std::string("const ") + cd->name + "*", true, false, cd };
}

// Look up type info by std::type_info. Returns nullptr if not registered.
//
// Hot-path fast cache: marshalling and reflection paths repeatedly look up
// the same handful of types (Vector2D, Vector3D, Color, ...) per frame.
// A tiny direct-mapped cache indexed by the low bits of the type_info pointer
// short-circuits the std::unordered_map<type_index, ...> lookup almost always.
inline const TypeInfo* LookupType(const std::type_info& ti) {
    struct CacheEntry { const std::type_info* key; const TypeInfo* val; };
    constexpr size_t CACHE_BITS = 4;            // 16-entry direct-mapped cache
    constexpr size_t CACHE_SIZE = 1u << CACHE_BITS;
    constexpr size_t CACHE_MASK = CACHE_SIZE - 1;
    static thread_local CacheEntry cache[CACHE_SIZE] = {};

    const std::type_info* tip = &ti;
    // Mix the pointer's high+low bits so adjacent type_info statics don't collide.
    size_t h = reinterpret_cast<uintptr_t>(tip);
    size_t slot = ((h >> 4) ^ (h >> 12)) & CACHE_MASK;
    CacheEntry& e = cache[slot];
    if (e.key == tip) return e.val;

    auto& reg = TypeRegistry();
    auto it = reg.find(std::type_index(ti));
    const TypeInfo* result = (it != reg.end()) ? &it->second : nullptr;
    e.key = tip;
    e.val = result;
    return result;
}

// Check if a type is a pointer (without parsing mangled names).
inline bool IsPointerType(const std::type_info& ti) {
    auto info = LookupType(ti);
    return info ? info->is_pointer : false;
}

// Check if a type is a reference (without parsing mangled names).
inline bool IsReferenceType(const std::type_info& ti) {
    auto info = LookupType(ti);
    return info ? info->is_reference : false;
}

// Check if a type is a pointer or reference.
inline bool IsIndirectionType(const std::type_info& ti) {
    auto info = LookupType(ti);
    return info ? (info->is_pointer || info->is_reference) : false;
}

// Get the ClassDesc for a type (follows pointers/references to the base class).
inline const ClassDesc* ClassForType(const std::type_info& ti) {
    auto info = LookupType(ti);
    return info ? info->classDesc : nullptr;
}

// Map a runtime type_info to FieldKind. Returns Complex for unrecognized types.
//
// Hot-path version: cache-friendly small linear scan over a static table
// of (type_info*, FieldKind) pairs. The previous implementation used a
// std::unordered_map<std::type_index, ...> which was a measurable hotspot
// in the script->native marshalling path.
inline FieldKind KindForType(const std::type_info& ti) {
    struct Entry { const std::type_info* ti; FieldKind kind; };
    static const Entry table[] = {
        {&typeid(float),       FieldKind::Float},
        {&typeid(int),         FieldKind::Int},
        {&typeid(double),      FieldKind::Double},
        {&typeid(bool),        FieldKind::Bool},
        {&typeid(uint8_t),     FieldKind::UInt8},
        {&typeid(uint16_t),    FieldKind::UInt16},
        {&typeid(uint32_t),    FieldKind::UInt32},
        {&typeid(std::size_t), FieldKind::SizeT},
    };
    const std::type_info* p = &ti;
    for (const auto& e : table) {
        if (e.ti == p) return e.kind;
    }
    return FieldKind::Complex;
}

} // namespace koilo

#else // !KL_HAS_RTTI - stub declarations for -fno-rtti builds (KSO shaders)

namespace koilo {
template<typename T>
inline void RegisterTypeInfo(const ClassDesc*) {}
} // namespace koilo

#endif // KL_HAS_RTTI
