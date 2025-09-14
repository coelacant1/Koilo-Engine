#pragma once

#include <type_traits>
#include "registry.hpp"
#include "reflect_make.hpp"
#include "global_registry.hpp"

#define PTX_FIELD(CLASS, MEMBER, DESC, MINv, MAXv) \
    {   #MEMBER, &typeid(decltype(std::declval<CLASS>().MEMBER)), \
        sizeof(decltype(std::declval<CLASS>().MEMBER)), \
        ptx::FieldAccess{ \
            +[](void* obj)->void* { return &static_cast<CLASS*>(obj)->MEMBER; }, \
            +[](const void* obj)->const void* { return &static_cast<const CLASS*>(obj)->MEMBER; } \
        }, \
        DESC, double(MINv), double(MAXv) }

#define PTX_BEGIN_FIELDS(CLASS) \
public: \
    static ptx::FieldList Fields() { \
        static const ptx::FieldDecl _fields[] = {

#define PTX_END_FIELDS \
        }; \
        return { _fields, sizeof(_fields)/sizeof(_fields[0]) }; \
    }

#define PTX_BEGIN_METHODS(CLASS) \
public: \
    static ptx::MethodList Methods() { \
        static const ptx::MethodDesc _methods[] = {

#define PTX_END_METHODS \
        }; \
        return { _methods, sizeof(_methods)/sizeof(_methods[0]) }; \
    }

#define PTX_METHOD_AUTO(CLASS, NAME, DESC) \
    ptx::make::MakeMethod<CLASS, &CLASS::NAME>(#NAME, DESC)

#define PTX_SMETHOD_AUTO(FN, DESC) \
    ptx::make::MakeSmethod<&FN>(#FN, DESC)

#define PTX_METHOD_OVLD(CLASS, NAME, RET, /*Args...*/ ...) \
    ptx::make::MakeMethod<CLASS, static_cast<RET (CLASS::*)(__VA_ARGS__)>(&CLASS::NAME)>(#NAME, nullptr)

#define PTX_METHOD_OVLD_CONST(CLASS, NAME, RET, /*Args...*/ ...) \
    ptx::make::MakeMethod<CLASS, static_cast<RET (CLASS::*)(__VA_ARGS__) const>(&CLASS::NAME)>(#NAME, nullptr)

#define PTX_CTOR0(CLASS) \
    ptx::make::MakeCtor<CLASS>(#CLASS "()")

#define PTX_CTOR(CLASS, /*Args...*/ ...) \
    ptx::make::MakeCtor<CLASS, __VA_ARGS__>(#CLASS "(" #__VA_ARGS__ ")")

#define PTX_BEGIN_DESCRIBE(CLASS) \
public: \
    static const ptx::ClassDesc& Describe() { \
        static const ptx::ConstructorDesc _ctors[] = {

#define PTX_END_DESCRIBE(CLASS) \
        }; \
        static const ptx::ClassDesc cd{ \
            #CLASS, \
            CLASS::Fields(), \
            CLASS::Methods(), \
            _ctors, sizeof(_ctors)/sizeof(_ctors[0]), \
            +[](void* p){ delete static_cast<CLASS*>(p); } \
        }; \
        static const ptx::AutoRegistrar _ptx_autoreg_##CLASS(&cd); \
        return cd; \
    }
