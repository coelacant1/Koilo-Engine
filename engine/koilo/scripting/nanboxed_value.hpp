// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file nanboxed_value.hpp
 * @brief 8-byte NaN-boxed tagged value for the KoiloScript bytecode VM.
 *
 * Encodes all KoiloScript value types in a single 64-bit word using
 * IEEE 754 NaN-boxing. Numbers are stored as raw doubles; all other
 * types are encoded in the quiet NaN payload space.
 *
 * Encoding:
 *   NUMBER:  Raw IEEE 754 double (NaN canonicalized to +qNaN)
 *   Tagged:  Bits [63:51] = negative quiet NaN prefix (0xFFF8 >> 48)
 *            Bits [50:48] = type tag (0-7)
 *            Bits [47:0]  = 48-bit payload (pointer or immediate)
 *
 * @date 23/02/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include "../registry/reflect_macros.hpp"

namespace koilo {
namespace scripting {

struct ScriptInstance;

// Heap-allocated array for NaN-boxed values
struct HeapArray {
    std::vector<class NanBoxedValue> elements;

    KL_BEGIN_FIELDS(HeapArray)
        KL_FIELD(HeapArray, elements, "Elements", 0, 0)
    KL_END_FIELDS

    KL_BEGIN_METHODS(HeapArray)
        /* No reflected methods. */
    KL_END_METHODS

    KL_BEGIN_DESCRIBE(HeapArray)
        /* No reflected ctors. */
    KL_END_DESCRIBE(HeapArray)

};

/**
 * @brief 8-byte NaN-boxed value. Replaces the ~125-byte Value struct on the VM stack.
 */
class NanBoxedValue {
public:
    // NaN-boxing constants
    static constexpr uint64_t QNAN_PREFIX   = 0xFFF8000000000000ULL;
    static constexpr uint64_t CANONICAL_NAN = 0x7FF8000000000000ULL;
    static constexpr uint64_t TAG_SHIFT     = 48;
    static constexpr uint64_t TAG_BITS      = 0x0007000000000000ULL;
    static constexpr uint64_t PAYLOAD_MASK  = 0x0000FFFFFFFFFFFFULL;

    static constexpr uint64_t TAG_NONE     = 0ULL;
    static constexpr uint64_t TAG_BOOL     = 1ULL;
    static constexpr uint64_t TAG_STRING   = 2ULL;
    static constexpr uint64_t TAG_ARRAY    = 3ULL;
    static constexpr uint64_t TAG_TABLE    = 4ULL;
    static constexpr uint64_t TAG_OBJECT   = 5ULL;
    static constexpr uint64_t TAG_FUNCTION = 6ULL;
    static constexpr uint64_t TAG_INSTANCE = 7ULL;

    NanBoxedValue() : bits_(QNAN_PREFIX) {} // default = NONE (tag 0)

    // Type checking
    bool IsTagged()   const { return (bits_ & QNAN_PREFIX) == QNAN_PREFIX; }
    bool IsNumber()   const { return !IsTagged(); }
    bool IsNone()     const { return bits_ == QNAN_PREFIX; } // tag 0, payload 0
    bool IsBool()     const { return IsTagged() && GetTag() == TAG_BOOL; }
    bool IsString()   const { return IsTagged() && GetTag() == TAG_STRING; }
    bool IsArray()    const { return IsTagged() && GetTag() == TAG_ARRAY; }
    bool IsTable()    const { return IsTagged() && GetTag() == TAG_TABLE; }
    bool IsObject()   const { return IsTagged() && GetTag() == TAG_OBJECT; }
    bool IsFunction() const { return IsTagged() && GetTag() == TAG_FUNCTION; }
    bool IsInstance() const { return IsTagged() && GetTag() == TAG_INSTANCE; }

    uint64_t GetTag() const { return (bits_ & TAG_BITS) >> TAG_SHIFT; }

    // Factories
    static NanBoxedValue Number(double d) {
        NanBoxedValue v;
        if (d != d) { v.bits_ = CANONICAL_NAN; return v; } // IEEE 754: NaN != NaN
        std::memcpy(&v.bits_, &d, sizeof(uint64_t));
        return v;
    }

    static NanBoxedValue Bool(bool b) {
        NanBoxedValue v;
        v.bits_ = QNAN_PREFIX | (TAG_BOOL << TAG_SHIFT) | (b ? 1ULL : 0ULL);
        return v;
    }

    static NanBoxedValue None() { return NanBoxedValue(); }

    static NanBoxedValue String(std::string* s) { return FromPtr(TAG_STRING, s); }
    static NanBoxedValue Array(HeapArray* a)     { return FromPtr(TAG_ARRAY, a); }
    static NanBoxedValue Table(void* mapPtr)     { return FromPtr(TAG_TABLE, mapPtr); }
    static NanBoxedValue Object(std::string* n)  { return FromPtr(TAG_OBJECT, n); }
    static NanBoxedValue Function(std::string* n){ return FromPtr(TAG_FUNCTION, n); }
    static NanBoxedValue Instance(ScriptInstance* i) { return FromPtr(TAG_INSTANCE, i); }

    // Accessors
    double AsNumber() const {
        double d; std::memcpy(&d, &bits_, sizeof(double)); return d;
    }
    bool AsBool()                  const { return (bits_ & PAYLOAD_MASK) != 0; }
    std::string* AsString()        const { return static_cast<std::string*>(GetPtr()); }
    HeapArray* AsArray()           const { return static_cast<HeapArray*>(GetPtr()); }
    void* AsTablePtr()             const { return GetPtr(); } // raw map pointer
    std::string* AsObjectName()    const { return static_cast<std::string*>(GetPtr()); }
    std::string* AsFunctionName()  const { return static_cast<std::string*>(GetPtr()); }
    ScriptInstance* AsInstance()   const { return static_cast<ScriptInstance*>(GetPtr()); }

    // Truthiness (matches KoiloScript semantics)
    bool IsTruthy() const {
        if (IsNumber()) return AsNumber() != 0.0;
        if (IsNone()) return false;
        if (IsBool()) return AsBool();
        if (IsString()) return AsString() && !AsString()->empty();
        return true;
    }

    uint64_t GetBits() const { return bits_; }

private:
    uint64_t bits_;

    void* GetPtr() const {
        return reinterpret_cast<void*>(static_cast<uintptr_t>(bits_ & PAYLOAD_MASK));
    }

    static NanBoxedValue FromPtr(uint64_t tag, void* ptr) {
        NanBoxedValue v;
        v.bits_ = QNAN_PREFIX | (tag << TAG_SHIFT) |
                  (reinterpret_cast<uintptr_t>(ptr) & PAYLOAD_MASK);
        return v;
    }

};

} // namespace scripting
} // namespace koilo
