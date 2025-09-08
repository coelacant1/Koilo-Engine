// registry.hpp
#pragma once
#include <cstddef>
#include <functional>
#include <typeinfo>

namespace ptx {

struct FieldAccess {
    std::function<void*(void*)>             get_ptr;
    std::function<const void*(const void*)> get_cptr;
};

struct FieldDecl {
    const char* name;
    const std::type_info* type;
    size_t size;
    FieldAccess access;
    const char* description;
    float minValue, maxValue;
};

struct MethodDesc {
    const char* name;
    const char* description;
    const std::type_info* return_type;
    const std::type_info* const* param_types;
    size_t param_count;

    std::function<void*(void* obj, void** args)> invoker;
};

struct FieldList  {
    const FieldDecl*  data;
    size_t count; 
};

struct MethodList {
    const MethodDesc* data;
    size_t count; 
};

} // namespace ptx
