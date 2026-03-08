// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstddef>
#include <cstdint>

// Portable RTTI detection - disabled by -fno-rtti (KSO shader builds)
#if defined(__GXX_RTTI) || defined(__cpp_rtti) || defined(_CPPRTTI)
  #define KL_HAS_RTTI 1
  #include <typeinfo>
#else
  #define KL_HAS_RTTI 0
  namespace std { class type_info; }  // forward-declare to keep FieldDecl layout
#endif

namespace koilo {

// Compile-time tag for field types, enabling switch-based dispatch
// instead of N sequential typeid comparisons.
enum class FieldKind : uint8_t {
    Float, Int, Double, Bool,
    UInt8, UInt16, UInt32, SizeT,
    Complex  // Requires ClassDesc lookup for sub-object navigation
};

// Map C++ type -> FieldKind at compile time.
template<typename T> struct FieldKindOf { static constexpr FieldKind value = FieldKind::Complex; };
template<> struct FieldKindOf<float>       { static constexpr FieldKind value = FieldKind::Float; };
template<> struct FieldKindOf<int>         { static constexpr FieldKind value = FieldKind::Int; };
template<> struct FieldKindOf<double>      { static constexpr FieldKind value = FieldKind::Double; };
template<> struct FieldKindOf<bool>        { static constexpr FieldKind value = FieldKind::Bool; };
template<> struct FieldKindOf<uint8_t>     { static constexpr FieldKind value = FieldKind::UInt8; };
template<> struct FieldKindOf<uint16_t>    { static constexpr FieldKind value = FieldKind::UInt16; };
template<> struct FieldKindOf<uint32_t>    { static constexpr FieldKind value = FieldKind::UInt32; };
template<> struct FieldKindOf<std::size_t> { static constexpr FieldKind value = FieldKind::SizeT; };

struct TypeSpan { // Span of std::type_info* 
    const std::type_info* const* data;
    std::size_t count;
};

#if KL_HAS_RTTI
template<class... Ts>
inline TypeSpan MakeTypeSpan() {
    static const std::type_info* kArr[] = { &typeid(Ts)... };
    return { kArr, sizeof...(Ts) };
}
#else
template<class... Ts>
inline TypeSpan MakeTypeSpan() {
    return { nullptr, 0 };
}
#endif

struct FieldAccess {
    void*       (*get_ptr)(void* obj);            // non-const pointer access
    const void* (*get_cptr)(const void* obj);     // const pointer access
};

struct FieldDecl {
    const char*               name;
    const std::type_info*     type;
    std::size_t               size;
    FieldAccess               access;
    const char*               description;
    double                    min_value;
    double                    max_value;
    FieldKind                 kind;       ///< Compile-time type tag for fast dispatch
};

struct MethodDesc {
    const char* name;
    const char* doc;
    const std::type_info* ret_type;
    koilo::TypeSpan arg_types;
    size_t argc;
    bool is_static;
    void* (*invoker)(void* self, void** args);
    const char* signature;
    std::size_t ret_size;
    void (*destroy_ret)(void*);
    bool ret_is_pointer;    ///< True if return type is a pointer (borrowed, caller doesn't own)
    bool ret_owns;          ///< True if return is a by-value object (caller owns, must delete)
};

struct ConstructorDesc {
    koilo::TypeSpan arg_types;
    const char* signature;
    void* (*invoker)(void** args);
    size_t min_argc;  ///< Minimum args (for default params); 0 = use arg_types.count
};

struct FieldList  { const FieldDecl*  data; std::size_t count; };
struct MethodList { const MethodDesc* data; std::size_t count; };

struct ClassDesc {
    const char* name;
    koilo::FieldList fields;
    koilo::MethodList methods;
    const ConstructorDesc* ctors;
    size_t ctor_count;
    size_t size;  /* sizeof(T) for the class */
    void (*destroy)(void*);
    void (*copy_construct)(void* dst, const void* src);  ///< Placement copy-construct src into dst
};

} // namespace koilo
