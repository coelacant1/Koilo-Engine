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

        // Numeric primitives: dispatch via FieldKind to avoid a chain of
        // ~8 sequential typeid pointer-comparisons.
        switch (KindForType(*targetType)) {
            case FieldKind::Float: {
                if (val.type == Value::Type::NUMBER && floatCount_ < MAX_SLOTS) { floats_[floatCount_] = static_cast<float>(val.numberValue); return &floats_[floatCount_++]; }
                if (val.type == Value::Type::BOOL   && floatCount_ < MAX_SLOTS) { floats_[floatCount_] = val.boolValue ? 1.0f : 0.0f; return &floats_[floatCount_++]; }
                return nullptr;
            }
            case FieldKind::Int: {
                if (val.type == Value::Type::NUMBER && intCount_ < MAX_SLOTS) { ints_[intCount_] = static_cast<int>(val.numberValue); return &ints_[intCount_++]; }
                if (val.type == Value::Type::BOOL   && intCount_ < MAX_SLOTS) { ints_[intCount_] = val.boolValue ? 1 : 0; return &ints_[intCount_++]; }
                return nullptr;
            }
            case FieldKind::Double: {
                if (val.type == Value::Type::NUMBER && doubleCount_ < MAX_SLOTS) { doubles_[doubleCount_] = val.numberValue; return &doubles_[doubleCount_++]; }
                if (val.type == Value::Type::BOOL   && doubleCount_ < MAX_SLOTS) { doubles_[doubleCount_] = val.boolValue ? 1.0 : 0.0; return &doubles_[doubleCount_++]; }
                return nullptr;
            }
            case FieldKind::Bool: {
                if (boolCount_ >= MAX_SLOTS) return nullptr;
                if (val.type == Value::Type::BOOL)   { bools_[boolCount_] = val.boolValue; return &bools_[boolCount_++]; }
                if (val.type == Value::Type::NUMBER) { bools_[boolCount_] = (val.numberValue != 0.0); return &bools_[boolCount_++]; }
                return nullptr;
            }
            case FieldKind::SizeT: {
                if (val.type == Value::Type::NUMBER && sizetCount_ < MAX_SLOTS) { sizets_[sizetCount_] = static_cast<std::size_t>(val.numberValue); return &sizets_[sizetCount_++]; }
                if (val.type == Value::Type::BOOL   && sizetCount_ < MAX_SLOTS) { sizets_[sizetCount_] = val.boolValue ? 1 : 0; return &sizets_[sizetCount_++]; }
                return nullptr;
            }
            case FieldKind::UInt16: {
                if (val.type == Value::Type::NUMBER && uint16Count_ < MAX_SLOTS) { uint16s_[uint16Count_] = static_cast<uint16_t>(val.numberValue); return &uint16s_[uint16Count_++]; }
                return nullptr;
            }
            case FieldKind::UInt8: {
                if (val.type == Value::Type::NUMBER && uint8Count_ < MAX_SLOTS) { uint8s_[uint8Count_] = static_cast<uint8_t>(val.numberValue); return &uint8s_[uint8Count_++]; }
                return nullptr;
            }
            case FieldKind::UInt32: {
                // Reuse int slots: uint32 is 4 bytes like int. Cast through uintptr_t avoids strict-aliasing issues.
                if (val.type == Value::Type::NUMBER && intCount_ < MAX_SLOTS) {
                    *reinterpret_cast<uint32_t*>(&ints_[intCount_]) = static_cast<uint32_t>(val.numberValue);
                    return &ints_[intCount_++];
                }
                return nullptr;
            }
            case FieldKind::Complex:
                break;  // Fall through to string/object/enum handling below.
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
            if (val.type == Value::Type::STRING) {
                // Zero-copy: the Value's own stringValue is alive for the
                // lifetime of this call (caller holds args by const-ref),
                // so we can hand the trampoline a pointer straight at it
                // instead of copying into our scratch slot. The cast drops
                // const because the marshaller's interface is `void*`; the
                // C++ function takes a const-reference and will not mutate.
                return const_cast<std::string*>(&val.stringValue);
            }
            return nullptr;
        }

        // Object reference
        if (val.type == Value::Type::OBJECT) {
            auto it = objects.find(val.objectName);
            void* instance = nullptr;
            if (it != objects.end() && it->second.instance) {
                instance = it->second.instance;
            } else {
                // Dotted path (e.g. "pose.orientation"): walk the field chain
                // from the base reflected object so sub-object references can
                // be passed as args without needing a pre-extracted copy.
                size_t dot = val.objectName.find('.');
                if (dot != std::string::npos) {
                    std::string baseName = val.objectName.substr(0, dot);
                    auto baseIt = objects.find(baseName);
                    if (baseIt != objects.end() && baseIt->second.instance && baseIt->second.classDesc) {
                        void* curInst = baseIt->second.instance;
                        const ClassDesc* curClass = baseIt->second.classDesc;
                        size_t pos = dot + 1;
                        while (pos < val.objectName.size() && curInst && curClass) {
                            size_t next = val.objectName.find('.', pos);
                            std::string fname = (next == std::string::npos)
                                ? val.objectName.substr(pos)
                                : val.objectName.substr(pos, next - pos);
                            const FieldDecl* f = ReflectionBridge::FindField(curClass, fname);
                            if (!f) { curInst = nullptr; break; }
                            curInst = ReflectionBridge::GetFieldPointer(curInst, f);
                            if (next == std::string::npos) break;
                            curClass = (f->kind == FieldKind::Complex && f->type) ? ClassForType(*f->type) : nullptr;
                            pos = next + 1;
                        }
                        instance = curInst;
                    }
                }
            }
            if (instance) {
                auto info = LookupType(*targetType);
                if (info && info->is_pointer) {
                    if (ptrCount_ < MAX_SLOTS) { ptrs_[ptrCount_] = instance; return &ptrs_[ptrCount_++]; }
                    return nullptr;
                }
                const char* tname = targetType->name();
                if (tname && tname[0] == 'P') {
                    if (ptrCount_ < MAX_SLOTS) { ptrs_[ptrCount_] = instance; return &ptrs_[ptrCount_++]; }
                    return nullptr;
                }
                return instance;
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
