// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../registry/reflect_macros.hpp"
/**
 * @file value_marshaller.hpp
 * @brief Unified type marshalling between KoiloScript Value and C++ types.
 * 
 * Consolidates the Value->C++ and C++->Value conversion logic that was
 * previously duplicated in ConstructObject(), CallReflectedMethod(),
 * and SetReflectedMemberValue().
 */

#include <koilo/scripting/koiloscript_engine.hpp>
#include <koilo/registry/type_registry.hpp>
#include <typeinfo>
#include <vector>
#include <unordered_map>
#include <string>
#include <cstdint>
#include <cstddef>

namespace koilo {
namespace scripting {

/**
 * @brief Marshals a batch of Value arguments to typed C++ pointers.
 * 
 * Owns storage for all converted values. Pointers remain valid for
 * the lifetime of this object.
 */
class ArgMarshaller {
public:
    // Reset all counters for reuse (no heap operations)
    void Clear() {
        floatCount_ = intCount_ = doubleCount_ = sizetCount_ = 0;
        uint16Count_ = uint8Count_ = boolCount_ = 0;
        cstrCount_ = stringCount_ = ptrCount_ = 0;
    }

    void* Marshal(const Value& val, const std::type_info* targetType,
                  const std::unordered_map<std::string, ReflectedObject>& objects) {
        if (!targetType) return nullptr;

        // Numeric types - accept NUMBER, coerce BOOL (true=1, false=0)
        if (*targetType == typeid(float)) {
            if (val.type == Value::Type::NUMBER && floatCount_ < MAX_SLOTS) { floats_[floatCount_] = static_cast<float>(val.numberValue); return &floats_[floatCount_++]; }
            if (val.type == Value::Type::BOOL && floatCount_ < MAX_SLOTS) { floats_[floatCount_] = val.boolValue ? 1.0f : 0.0f; return &floats_[floatCount_++]; }
            return nullptr;
        }
        if (*targetType == typeid(int)) {
            if (val.type == Value::Type::NUMBER && intCount_ < MAX_SLOTS) { ints_[intCount_] = static_cast<int>(val.numberValue); return &ints_[intCount_++]; }
            if (val.type == Value::Type::BOOL && intCount_ < MAX_SLOTS) { ints_[intCount_] = val.boolValue ? 1 : 0; return &ints_[intCount_++]; }
            return nullptr;
        }
        if (*targetType == typeid(double)) {
            if (val.type == Value::Type::NUMBER && doubleCount_ < MAX_SLOTS) { doubles_[doubleCount_] = val.numberValue; return &doubles_[doubleCount_++]; }
            if (val.type == Value::Type::BOOL && doubleCount_ < MAX_SLOTS) { doubles_[doubleCount_] = val.boolValue ? 1.0 : 0.0; return &doubles_[doubleCount_++]; }
            return nullptr;
        }
        if (*targetType == typeid(std::size_t)) {
            if (val.type == Value::Type::NUMBER && sizetCount_ < MAX_SLOTS) { sizets_[sizetCount_] = static_cast<std::size_t>(val.numberValue); return &sizets_[sizetCount_++]; }
            if (val.type == Value::Type::BOOL && sizetCount_ < MAX_SLOTS) { sizets_[sizetCount_] = val.boolValue ? 1 : 0; return &sizets_[sizetCount_++]; }
            return nullptr;
        }
        if (*targetType == typeid(uint16_t)) {
            if (val.type == Value::Type::NUMBER && uint16Count_ < MAX_SLOTS) { uint16s_[uint16Count_] = static_cast<uint16_t>(val.numberValue); return &uint16s_[uint16Count_++]; }
            return nullptr;
        }
        if (*targetType == typeid(uint8_t)) {
            if (val.type == Value::Type::NUMBER && uint8Count_ < MAX_SLOTS) { uint8s_[uint8Count_] = static_cast<uint8_t>(val.numberValue); return &uint8s_[uint8Count_++]; }
            return nullptr;
        }
        if (*targetType == typeid(bool)) {
            if (boolCount_ >= MAX_SLOTS) return nullptr;
            if (val.type == Value::Type::BOOL) { bools_[boolCount_] = val.boolValue; return &bools_[boolCount_++]; }
            if (val.type == Value::Type::NUMBER) { bools_[boolCount_] = (val.numberValue != 0.0); return &bools_[boolCount_++]; }
            return nullptr;
        }

        // String types - no silent coercion
        if (*targetType == typeid(const char*)) {
            if (val.type == Value::Type::STRING && cstrCount_ < MAX_SLOTS) {
                cstrs_[cstrCount_] = val.stringValue.c_str();
                return &cstrs_[cstrCount_++];
            }
            return nullptr;
        }
        if (*targetType == typeid(std::string)) {
            if (val.type == Value::Type::STRING && stringCount_ < MAX_SLOTS) { strings_[stringCount_] = val.stringValue; return &strings_[stringCount_++]; }
            return nullptr;
        }

        // Object reference
        if (val.type == Value::Type::OBJECT) {
            auto it = objects.find(val.objectName);
            if (it != objects.end() && it->second.instance) {
                auto info = LookupType(*targetType);
                if (info && info->is_pointer) {
                    if (ptrCount_ < MAX_SLOTS) { ptrs_[ptrCount_] = it->second.instance; return &ptrs_[ptrCount_++]; }
                    return nullptr;
                }
                // Fallback: GCC mangling check for pointer types not in registry
                std::string tname = targetType->name();
                if (!tname.empty() && tname[0] == 'P') {
                    if (ptrCount_ < MAX_SLOTS) { ptrs_[ptrCount_] = it->second.instance; return &ptrs_[ptrCount_++]; }
                    return nullptr;
                }
                // Value or reference parameter - return instance directly
                return it->second.instance;
            }
            return nullptr;
        }

        // Enum fallback: treat as int
        if (val.type == Value::Type::NUMBER && intCount_ < MAX_SLOTS) {
            ints_[intCount_] = static_cast<int>(val.numberValue);
            return &ints_[intCount_++];
        }

        return nullptr;
    }

    bool MarshalAll(const std::vector<Value>& args,
                    const std::type_info* const* paramTypes, size_t count,
                    const std::unordered_map<std::string, ReflectedObject>& objects,
                    std::vector<void*>& argPtrs) {
        Clear();
        argPtrs.resize(count);
        for (size_t i = 0; i < count; ++i) {
            argPtrs[i] = Marshal(args[i], paramTypes[i], objects);
            if (!argPtrs[i]) return false;
        }
        return true;
    }

    void Reserve(size_t /*n*/) {}

private:
    // Fixed arrays: no heap allocation, stable pointers across Marshal calls.
    // 16 slots per type covers all practical argument counts.
    static constexpr size_t MAX_SLOTS = 16;
    float floats_[MAX_SLOTS]{}; size_t floatCount_ = 0;
    int ints_[MAX_SLOTS]{}; size_t intCount_ = 0;
    double doubles_[MAX_SLOTS]{}; size_t doubleCount_ = 0;
    std::size_t sizets_[MAX_SLOTS]{}; size_t sizetCount_ = 0;
    uint16_t uint16s_[MAX_SLOTS]{}; size_t uint16Count_ = 0;
    uint8_t uint8s_[MAX_SLOTS]{}; size_t uint8Count_ = 0;
    bool bools_[MAX_SLOTS]{}; size_t boolCount_ = 0;
    const char* cstrs_[MAX_SLOTS]{}; size_t cstrCount_ = 0;
    std::string strings_[MAX_SLOTS]; size_t stringCount_ = 0;
    void* ptrs_[MAX_SLOTS]{}; size_t ptrCount_ = 0;

    KL_BEGIN_FIELDS(ArgMarshaller)
        /* ArgMarshaller is engine-internal, no script-visible fields */
    KL_END_FIELDS

    KL_BEGIN_METHODS(ArgMarshaller)
        /* ArgMarshaller is engine-internal, no script-visible methods */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(ArgMarshaller)
        /* No reflected ctors. */
    KL_END_DESCRIBE(ArgMarshaller)

};

/**
 * @brief Convert a C++ return value (void*) to a script Value.
 */
inline Value MarshalReturnValue(void* result, const std::type_info* retType, bool ownsResult = true) {
    if (!result || !retType) return Value();

    // Handle std::string returns (value return: BoxReturn allocates; ref return: raw pointer)
    if (*retType == typeid(std::string)) {
        auto* str = static_cast<std::string*>(result);
        Value v(*str);
        if (ownsResult) delete str;
        return v;
    }

    // Handle const char* returns (pointer to string literal or internal buffer)
    if (*retType == typeid(const char*)) {
        auto cstr = static_cast<const char*>(result);
        Value v(cstr ? std::string(cstr) : std::string());
        return v;
    }

    switch (KindForType(*retType)) {
        case FieldKind::Float: {
            Value v(static_cast<double>(*static_cast<float*>(result)));
            if (ownsResult) ::operator delete(result);
            return v;
        }
        case FieldKind::Int: {
            Value v(static_cast<double>(*static_cast<int*>(result)));
            if (ownsResult) ::operator delete(result);
            return v;
        }
        case FieldKind::Double: {
            Value v(*static_cast<double*>(result));
            if (ownsResult) ::operator delete(result);
            return v;
        }
        case FieldKind::Bool: {
            Value v;
            v.type = Value::Type::BOOL;
            v.boolValue = *static_cast<bool*>(result);
            if (ownsResult) ::operator delete(result);
            return v;
        }
        case FieldKind::SizeT: {
            Value v(static_cast<double>(*static_cast<std::size_t*>(result)));
            if (ownsResult) ::operator delete(result);
            return v;
        }
        case FieldKind::UInt16: {
            Value v(static_cast<double>(*static_cast<uint16_t*>(result)));
            if (ownsResult) ::operator delete(result);
            return v;
        }
        case FieldKind::UInt8: {
            Value v(static_cast<double>(*static_cast<uint8_t*>(result)));
            if (ownsResult) ::operator delete(result);
            return v;
        }
        case FieldKind::UInt32: {
            Value v(static_cast<double>(*static_cast<uint32_t*>(result)));
            if (ownsResult) ::operator delete(result);
            return v;
        }
        case FieldKind::Complex: break;
    }

    return Value(); // Complex type - caller handles object wrapping
}

/**
 * @brief Set a reflected field from a Value, handling type conversion.
 * @return true if the field was set successfully
 */
inline bool MarshalToField(void* instance, const FieldDecl* field, const Value& value) {
    if (!instance || !field) return false;

    if (value.type == Value::Type::NUMBER) {
        void* ptr = field->access.get_ptr(instance);
        if (!ptr) return false;
        switch (field->kind) {
            case FieldKind::Float:  *static_cast<float*>(ptr) = static_cast<float>(value.numberValue); return true;
            case FieldKind::Int:    *static_cast<int*>(ptr) = static_cast<int>(value.numberValue); return true;
            case FieldKind::Double: *static_cast<double*>(ptr) = value.numberValue; return true;
            case FieldKind::Bool:   *static_cast<bool*>(ptr) = (value.numberValue != 0.0); return true;
            case FieldKind::UInt8:  *static_cast<uint8_t*>(ptr) = static_cast<uint8_t>(value.numberValue); return true;
            case FieldKind::UInt16: *static_cast<uint16_t*>(ptr) = static_cast<uint16_t>(value.numberValue); return true;
            case FieldKind::UInt32: *static_cast<uint32_t*>(ptr) = static_cast<uint32_t>(value.numberValue); return true;
            case FieldKind::SizeT:  *static_cast<std::size_t*>(ptr) = static_cast<std::size_t>(value.numberValue); return true;
            default: return false;
        }
    }
    if (value.type == Value::Type::BOOL && field->kind == FieldKind::Bool) {
        void* ptr = field->access.get_ptr(instance);
        if (!ptr) return false;
        *static_cast<bool*>(ptr) = value.boolValue;
        return true;
    }

    return false;
}

} // namespace scripting
} // namespace koilo
