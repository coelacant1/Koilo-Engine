#pragma once
#include <type_traits>
#include <utility>
#include "reflectable.hpp"
#include "reflect_make.hpp"

// Arrays as class scope statics
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

// Fields
#define PTX_FIELD(CLASS, MEMBER, DESC, MIN, MAX) \
    ptx::MakeField(#MEMBER, &CLASS::MEMBER, DESC, MIN, MAX)

// Methods (member & static)
#define PTX_METHOD(CLASS, NAME, DESC) \
    ptx::MakeMethod(#NAME, DESC, &CLASS::NAME)

#define PTX_SMETHOD(CLASS, NAME, DESC) \
    ptx::MakeSmethod(#NAME, DESC, &CLASS::NAME)
