// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <type_traits>
#include <cstring>
#include <new>
#include <utility>
#include "registry.hpp"
#include "reflect_make.hpp"
#include "global_registry.hpp"
#include "type_registry.hpp"

namespace koilo::detail {
    // Explicit opt-in: most reflected types use trivial-copyable detection
    // (memcpy) or no copy at all (nullptr). Types that need real copy
    // semantics (e.g. ones holding std::vector / std::string) opt in via
    // KL_COPYABLE(Type) below, which specialises this helper.
    //
    // Why opt-in: GCC 15 incorrectly reports types holding std::mutex /
    // std::once_flag / std::atomic as copy-constructible in both
    // std::is_copy_constructible AND expression-SFINAE contexts, so we
    // can't auto-detect safely.
    template<typename T>
    struct CopyConstructHelper {
        static void Invoke(void* dst, const void* src) {
            std::memcpy(dst, src, sizeof(T));
        }
        static constexpr void (*fn)(void*, const void*) =
            std::is_trivially_copyable_v<T> ? &Invoke : nullptr;
    };

    // Default copy-assign: memcpy for trivially-destructible types (safe
    // since they manage no resources). Types that own resources (vectors,
    // strings, shared_ptrs) must opt in via KL_END_DESCRIBE_COPYABLE.
    // We use is_trivially_destructible (not is_copy_assignable) because
    // GCC 15 falsely reports types holding std::unique_ptr / std::mutex
    // as copy-assignable in SFINAE contexts.
    template<typename T>
    struct CopyAssignHelper {
        static void Invoke(void* dst, const void* src) {
            std::memcpy(dst, src, sizeof(T));
        }
        static constexpr void (*fn)(void*, const void*) =
            std::is_trivially_destructible_v<T> ? &Invoke : nullptr;
    };
} // namespace koilo::detail

// Opt a non-trivially-copyable reflected type in to proper copy semantics.
// Use INSIDE the class definition in place of KL_END_DESCRIBE - it expands
// the same way but installs a placement-new copy_construct instead of
// memcpy. Required for any reflected struct holding std::vector,
// std::string, std::shared_ptr, etc. that script code may need to copy
// (e.g. via `target.field = source` assignment).
#define KL_END_DESCRIBE_COPYABLE(CLASS) \
        }; \
        static const koilo::ClassDesc cd{ \
            #CLASS, \
            CLASS::Fields(), \
            CLASS::Methods(), \
            _ctors, sizeof(_ctors)/sizeof(_ctors[0]), \
            sizeof(CLASS), \
            +[](void* p){ delete static_cast<CLASS*>(p); }, \
            +[](void* dst, const void* src){ \
                ::new (dst) CLASS(*static_cast<const CLASS*>(src)); \
            }, \
            +[](void* dst, const void* src){ \
                *static_cast<CLASS*>(dst) = *static_cast<const CLASS*>(src); \
            } \
        }; \
        static const koilo::AutoRegistrar _koilo_autoreg_##CLASS(&cd); \
        KL_REGISTER_TYPE_INFO(CLASS, cd) \
        return cd; \
    }

#if KL_HAS_RTTI
#define KL_FIELD(CLASS, MEMBER, DESC, MINv, MAXv) \
    {   #MEMBER, &typeid(decltype(std::declval<CLASS>().MEMBER)), \
        sizeof(decltype(std::declval<CLASS>().MEMBER)), \
        koilo::FieldAccess{ \
            +[](void* obj)->void* { return const_cast<void*>(static_cast<const void*>(&static_cast<CLASS*>(obj)->MEMBER)); }, \
            +[](const void* obj)->const void* { return static_cast<const void*>(&static_cast<const CLASS*>(obj)->MEMBER); } \
        }, \
        DESC, double(MINv), double(MAXv), \
        koilo::FieldKindOf<std::remove_cv_t<decltype(std::declval<CLASS>().MEMBER)>>::value, \
        koilo::FieldHint::None, nullptr }

#define KL_FIELD_EX(CLASS, MEMBER, DESC, MINv, MAXv, HINT, CATEGORY) \
    {   #MEMBER, &typeid(decltype(std::declval<CLASS>().MEMBER)), \
        sizeof(decltype(std::declval<CLASS>().MEMBER)), \
        koilo::FieldAccess{ \
            +[](void* obj)->void* { return const_cast<void*>(static_cast<const void*>(&static_cast<CLASS*>(obj)->MEMBER)); }, \
            +[](const void* obj)->const void* { return static_cast<const void*>(&static_cast<const CLASS*>(obj)->MEMBER); } \
        }, \
        DESC, double(MINv), double(MAXv), \
        koilo::FieldKindOf<std::remove_cv_t<decltype(std::declval<CLASS>().MEMBER)>>::value, \
        koilo::FieldHint::HINT, CATEGORY }
#else
#define KL_FIELD(CLASS, MEMBER, DESC, MINv, MAXv) \
    {   #MEMBER, nullptr, \
        sizeof(decltype(std::declval<CLASS>().MEMBER)), \
        koilo::FieldAccess{ \
            +[](void* obj)->void* { return const_cast<void*>(static_cast<const void*>(&static_cast<CLASS*>(obj)->MEMBER)); }, \
            +[](const void* obj)->const void* { return static_cast<const void*>(&static_cast<const CLASS*>(obj)->MEMBER); } \
        }, \
        DESC, double(MINv), double(MAXv), \
        koilo::FieldKindOf<std::remove_cv_t<decltype(std::declval<CLASS>().MEMBER)>>::value, \
        koilo::FieldHint::None, nullptr }

#define KL_FIELD_EX(CLASS, MEMBER, DESC, MINv, MAXv, HINT, CATEGORY) \
    {   #MEMBER, nullptr, \
        sizeof(decltype(std::declval<CLASS>().MEMBER)), \
        koilo::FieldAccess{ \
            +[](void* obj)->void* { return const_cast<void*>(static_cast<const void*>(&static_cast<CLASS*>(obj)->MEMBER)); }, \
            +[](const void* obj)->const void* { return static_cast<const void*>(&static_cast<const CLASS*>(obj)->MEMBER); } \
        }, \
        DESC, double(MINv), double(MAXv), \
        koilo::FieldKindOf<std::remove_cv_t<decltype(std::declval<CLASS>().MEMBER)>>::value, \
        koilo::FieldHint::HINT, CATEGORY }
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
            koilo::detail::CopyConstructHelper<CLASS>::fn, \
            koilo::detail::CopyAssignHelper<CLASS>::fn \
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
