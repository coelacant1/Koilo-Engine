// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "registry.hpp"

#if KL_HAS_RTTI
#include <typeindex>
#include <unordered_map>
#include <string>

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
inline const TypeInfo* LookupType(const std::type_info& ti) {
    auto& reg = TypeRegistry();
    auto it = reg.find(std::type_index(ti));
    return (it != reg.end()) ? &it->second : nullptr;
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
inline FieldKind KindForType(const std::type_info& ti) {
    static const auto& map = *new std::unordered_map<std::type_index, FieldKind>{
        {std::type_index(typeid(float)),       FieldKind::Float},
        {std::type_index(typeid(int)),         FieldKind::Int},
        {std::type_index(typeid(double)),      FieldKind::Double},
        {std::type_index(typeid(bool)),        FieldKind::Bool},
        {std::type_index(typeid(uint8_t)),     FieldKind::UInt8},
        {std::type_index(typeid(uint16_t)),    FieldKind::UInt16},
        {std::type_index(typeid(uint32_t)),    FieldKind::UInt32},
        {std::type_index(typeid(std::size_t)), FieldKind::SizeT},
    };
    auto it = map.find(std::type_index(ti));
    return (it != map.end()) ? it->second : FieldKind::Complex;
}

} // namespace koilo

#else // !KL_HAS_RTTI - stub declarations for -fno-rtti builds (KSO shaders)

namespace koilo {
template<typename T>
inline void RegisterTypeInfo(const ClassDesc*) {}
} // namespace koilo

#endif // KL_HAS_RTTI
