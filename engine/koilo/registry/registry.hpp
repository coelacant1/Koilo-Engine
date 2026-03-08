#pragma once
#include <cstddef>
#include <typeinfo>

namespace ptx {

struct TypeSpan { // Span of std::type_info* 
    const std::type_info* const* data;
    std::size_t count;
};

template<class... Ts>
inline TypeSpan MakeTypeSpan() {
    static const std::type_info* kArr[] = { &typeid(Ts)... };
    return { kArr, sizeof...(Ts) };
}

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
};

struct MethodDesc {
    const char* name;
    const char* doc;
    const std::type_info* ret_type;
    ptx::TypeSpan arg_types;
    size_t argc;
    bool is_static;
    void* (*invoker)(void* self, void** args);
    const char* signature;
    std::size_t ret_size;
    void (*destroy_ret)(void*);
};

struct ConstructorDesc {
    ptx::TypeSpan arg_types;
    const char* signature;
    void* (*invoker)(void** args);
};

struct FieldList  { const FieldDecl*  data; std::size_t count; };
struct MethodList { const MethodDesc* data; std::size_t count; };

struct ClassDesc {
    const char* name;
    ptx::FieldList fields;
    ptx::MethodList methods;
    const ConstructorDesc* ctors;
    size_t ctor_count;
    void (*destroy)(void*);
};

} // namespace ptx
