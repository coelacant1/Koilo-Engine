// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <type_traits>
#include <cstring>
#include "registry.hpp"
#include "reflect_make.hpp"
#include "global_registry.hpp"
#include "type_registry.hpp"

namespace koilo::detail {
    // GCC 15 incorrectly reports types with deleted implicit copy ctors as
    // copy-constructible in SFINAE contexts.  Use trivially-copyable as a
    // safe, portable proxy: trivially-copyable types get memcpy (always
    // correct), everything else gets nullptr (caller already falls back to
    // memcpy).  Proper copy_construct for non-trivial types can be added
    // later via an explicit KL_COPYABLE opt-in macro.
    template<typename T>
    struct CopyConstructHelper {
        static void Invoke(void* dst, const void* src) {
            std::memcpy(dst, src, sizeof(T));
        }
        static constexpr void (*fn)(void*, const void*) =
            std::is_trivially_copyable_v<T> ? &Invoke : nullptr;
    };
} // namespace koilo::detail

#if KL_HAS_RTTI
#define KL_FIELD(CLASS, MEMBER, DESC, MINv, MAXv) \
    {   #MEMBER, &typeid(decltype(std::declval<CLASS>().MEMBER)), \
        sizeof(decltype(std::declval<CLASS>().MEMBER)), \
        koilo::FieldAccess{ \
            +[](void* obj)->void* { return const_cast<void*>(static_cast<const void*>(&static_cast<CLASS*>(obj)->MEMBER)); }, \
            +[](const void* obj)->const void* { return static_cast<const void*>(&static_cast<const CLASS*>(obj)->MEMBER); } \
        }, \
        DESC, double(MINv), double(MAXv), \
        koilo::FieldKindOf<std::remove_cv_t<decltype(std::declval<CLASS>().MEMBER)>>::value }
#else
#define KL_FIELD(CLASS, MEMBER, DESC, MINv, MAXv) \
    {   #MEMBER, nullptr, \
        sizeof(decltype(std::declval<CLASS>().MEMBER)), \
        koilo::FieldAccess{ \
            +[](void* obj)->void* { return const_cast<void*>(static_cast<const void*>(&static_cast<CLASS*>(obj)->MEMBER)); }, \
            +[](const void* obj)->const void* { return static_cast<const void*>(&static_cast<const CLASS*>(obj)->MEMBER); } \
        }, \
        DESC, double(MINv), double(MAXv), \
        koilo::FieldKindOf<std::remove_cv_t<decltype(std::declval<CLASS>().MEMBER)>>::value }
#endif

#define KL_BEGIN_FIELDS(CLASS) \
public: \
    static koilo::FieldList Fields() { \
        static const koilo::FieldDecl _fields[] = {

#define KL_END_FIELDS \
        }; \
        return { _fields, sizeof(_fields)/sizeof(_fields[0]) }; \
    }

#define KL_BEGIN_METHODS(CLASS) \
public: \
    static koilo::MethodList Methods() { \
        static const koilo::MethodDesc _methods[] = {

#define KL_END_METHODS \
        }; \
        return { _methods, sizeof(_methods)/sizeof(_methods[0]) }; \
    }

// -- Out-of-class reflection macros -------------------------------------------
// Use KL_DECLARE_* in the class body (header) and KL_DEFINE_* in a .cpp.
// This gives Methods()/Fields()/Describe() a single strong definition,
// eliminating COMDAT races when the header changes.

#define KL_DECLARE_FIELDS(CLASS) \
public: \
    static koilo::FieldList Fields();

#define KL_DECLARE_METHODS(CLASS) \
public: \
    static koilo::MethodList Methods();

#define KL_DECLARE_DESCRIBE(CLASS) \
public: \
    static const koilo::ClassDesc& Describe();

#define KL_DEFINE_FIELDS(CLASS) \
koilo::FieldList CLASS::Fields() { \
    static const koilo::FieldDecl _fields[] = {

#define KL_DEFINE_METHODS(CLASS) \
koilo::MethodList CLASS::Methods() { \
    static const koilo::MethodDesc _methods[] = {

#define KL_DEFINE_DESCRIBE(CLASS) \
const koilo::ClassDesc& CLASS::Describe() { \
    static const koilo::ConstructorDesc _ctors[] = {

#define KL_METHOD_AUTO(CLASS, NAME, DESC) \
    koilo::make::MakeMethod<CLASS, &CLASS::NAME>(#NAME, DESC)

#define KL_SMETHOD_AUTO(FN, DESC) \
    koilo::make::MakeSmethod<&FN>(#FN, DESC)

#define KL_METHOD_OVLD(CLASS, NAME, RET, /*Args...*/ ...) \
    koilo::make::MakeMethod<CLASS, static_cast<RET (CLASS::*)(__VA_ARGS__)>(&CLASS::NAME)>(#NAME, nullptr)

#define KL_METHOD_OVLD_CONST(CLASS, NAME, RET, /*Args...*/ ...) \
    koilo::make::MakeMethod<CLASS, static_cast<RET (CLASS::*)(__VA_ARGS__) const>(&CLASS::NAME)>(#NAME, nullptr)

#define KL_METHOD_OVLD0(CLASS, NAME, RET) \
    koilo::make::MakeMethod<CLASS, static_cast<RET (CLASS::*)()>(&CLASS::NAME)>(#NAME, nullptr)

#define KL_METHOD_OVLD_CONST0(CLASS, NAME, RET) \
    koilo::make::MakeMethod<CLASS, static_cast<RET (CLASS::*)() const>(&CLASS::NAME)>(#NAME, nullptr)

#define KL_SMETHOD_OVLD(CLASS, NAME, RET, /*Args...*/ ...) \
    koilo::make::MakeSmethod<static_cast<RET (*)(__VA_ARGS__)>(&CLASS::NAME)>(#NAME, nullptr)

#define KL_SMETHOD_OVLD0(CLASS, NAME, RET) \
    koilo::make::MakeSmethod<static_cast<RET (*)()>(&CLASS::NAME)>(#NAME, nullptr)

#define KL_CTOR0(CLASS) \
    koilo::make::MakeCtor<CLASS>(#CLASS "()")

#define KL_CTOR(CLASS, /*Args...*/ ...) \
    koilo::make::MakeCtor<CLASS, __VA_ARGS__>(#CLASS "(" #__VA_ARGS__ ")")

#define KL_CTOR_DEFAULTS(CLASS, MIN_ARGS, /*Args...*/ ...) \
    koilo::make::MakeCtorDefaults<MIN_ARGS, CLASS, __VA_ARGS__>(#CLASS "(" #__VA_ARGS__ ")")

// Compile-time documentation: reflection dispatches methods via
// static_cast<Derived*>(void*).  This is always safe because reflected
// methods are registered on the concrete class (not a base), and the
// void* originates from `new Derived(...)`.  For multiple-inheritance
// classes, keep the scriptable interface as the FIRST base to avoid
// any pointer adjustment surprises.
///
// Usage: place in any multiply-inherited class with Describe().
#define KL_REFLECTION_SAFE(DERIVED, FIRST_BASE) \
    static_assert(sizeof(DERIVED) >= sizeof(FIRST_BASE), \
        #FIRST_BASE " must be first base of " #DERIVED " for reflection safety")

#define KL_BEGIN_DESCRIBE(CLASS) \
public: \
    static const koilo::ClassDesc& Describe() { \
        static const koilo::ConstructorDesc _ctors[] = {

#define KL_END_DESCRIBE(CLASS) \
        }; \
        static const koilo::ClassDesc cd{ \
            #CLASS, \
            CLASS::Fields(), \
            CLASS::Methods(), \
            _ctors, sizeof(_ctors)/sizeof(_ctors[0]), \
            sizeof(CLASS), \
            +[](void* p){ delete static_cast<CLASS*>(p); }, \
            koilo::detail::CopyConstructHelper<CLASS>::fn \
        }; \
        static const koilo::AutoRegistrar _koilo_autoreg_##CLASS(&cd); \
        KL_REGISTER_TYPE_INFO(CLASS, cd) \
        return cd; \
    }

#if KL_HAS_RTTI
#define KL_REGISTER_TYPE_INFO(CLASS, cd) \
    static const bool _koilo_typereg_##CLASS = (koilo::RegisterTypeInfo<CLASS>(&cd), true); \
    (void)_koilo_typereg_##CLASS;
#else
#define KL_REGISTER_TYPE_INFO(CLASS, cd)
#endif
