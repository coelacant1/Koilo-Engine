#include "reflect.h"

#include <cstring>
#include <string>
#include <typeinfo>

#include "../ptx/registry/global_registry.hpp"
#include "../ptx/registry/registry.hpp"
#include "../ptx/registry/demangle.hpp"

namespace {

inline const ptx::ClassDesc* to_cpp(const ptx_class_desc* cls) {
    return reinterpret_cast<const ptx::ClassDesc*>(cls);
}

inline const ptx::FieldDecl* to_cpp(const ptx_field_decl* field) {
    return reinterpret_cast<const ptx::FieldDecl*>(field);
}

inline const ptx::MethodDesc* to_cpp(const ptx_method_desc* method) {
    return reinterpret_cast<const ptx::MethodDesc*>(method);
}

inline const ptx::ConstructorDesc* to_cpp(const ptx_constructor_desc* ctor) {
    return reinterpret_cast<const ptx::ConstructorDesc*>(ctor);
}

inline const ptx_class_desc* from_cpp(const ptx::ClassDesc* cls) {
    return reinterpret_cast<const ptx_class_desc*>(cls);
}

inline const ptx_field_decl* from_cpp(const ptx::FieldDecl* field) {
    return reinterpret_cast<const ptx_field_decl*>(field);
}

inline const ptx_method_desc* from_cpp(const ptx::MethodDesc* method) {
    return reinterpret_cast<const ptx_method_desc*>(method);
}

inline const ptx_constructor_desc* from_cpp(const ptx::ConstructorDesc* ctor) {
    return reinterpret_cast<const ptx_constructor_desc*>(ctor);
}

inline const std::type_info* to_type(const void* ptr) {
    return reinterpret_cast<const std::type_info*>(ptr);
}

inline const void* from_type(const std::type_info* ti) {
    return reinterpret_cast<const void*>(ti);
}

thread_local std::string g_pretty_name;

} // namespace

extern "C" {

size_t ptx_registry_class_count(void) {
    return ptx::GlobalClassRegistry().size();
}

const ptx_class_desc* ptx_registry_class_at(size_t index) {
    auto& reg = ptx::GlobalClassRegistry();
    if (index >= reg.size()) {
        return nullptr;
    }
    return from_cpp(reg[index]);
}

const ptx_class_desc* ptx_registry_find_class(const char* name) {
    if (!name) {
        return nullptr;
    }
    auto& reg = ptx::GlobalClassRegistry();
    for (const auto* cls : reg) {
        if (cls && cls->name && std::strcmp(cls->name, name) == 0) {
            return from_cpp(cls);
        }
    }
    return nullptr;
}

const char* ptx_class_name(const ptx_class_desc* cls) {
    const auto* cpp = to_cpp(cls);
    return cpp ? cpp->name : nullptr;
}

size_t ptx_class_field_count(const ptx_class_desc* cls) {
    const auto* cpp = to_cpp(cls);
    return cpp ? cpp->fields.count : 0;
}

const ptx_field_decl* ptx_class_field_at(const ptx_class_desc* cls, size_t index) {
    const auto* cpp = to_cpp(cls);
    if (!cpp || index >= cpp->fields.count) {
        return nullptr;
    }
    return from_cpp(&cpp->fields.data[index]);
}

const ptx_field_decl* ptx_class_find_field(const ptx_class_desc* cls, const char* name) {
    if (!name) {
        return nullptr;
    }
    const auto* cpp = to_cpp(cls);
    if (!cpp) {
        return nullptr;
    }
    for (size_t i = 0; i < cpp->fields.count; ++i) {
        const auto& field = cpp->fields.data[i];
        if (field.name && std::strcmp(field.name, name) == 0) {
            return from_cpp(&field);
        }
    }
    return nullptr;
}

size_t ptx_class_method_count(const ptx_class_desc* cls) {
    const auto* cpp = to_cpp(cls);
    return cpp ? cpp->methods.count : 0;
}

const ptx_method_desc* ptx_class_method_at(const ptx_class_desc* cls, size_t index) {
    const auto* cpp = to_cpp(cls);
    if (!cpp || index >= cpp->methods.count) {
        return nullptr;
    }
    return from_cpp(&cpp->methods.data[index]);
}

const ptx_method_desc* ptx_class_find_method(const ptx_class_desc* cls, const char* name) {
    if (!name) {
        return nullptr;
    }
    const auto* cpp = to_cpp(cls);
    if (!cpp) {
        return nullptr;
    }
    for (size_t i = 0; i < cpp->methods.count; ++i) {
        const auto& method = cpp->methods.data[i];
        if (method.name && std::strcmp(method.name, name) == 0) {
            return from_cpp(&method);
        }
    }
    return nullptr;
}

size_t ptx_class_constructor_count(const ptx_class_desc* cls) {
    const auto* cpp = to_cpp(cls);
    return cpp ? cpp->ctor_count : 0;
}

const ptx_constructor_desc* ptx_class_constructor_at(const ptx_class_desc* cls, size_t index) {
    const auto* cpp = to_cpp(cls);
    if (!cpp || index >= cpp->ctor_count) {
        return nullptr;
    }
    return from_cpp(&cpp->ctors[index]);
}

void ptx_class_destroy_instance(const ptx_class_desc* cls, void* instance) {
    const auto* cpp = to_cpp(cls);
    if (cpp && cpp->destroy && instance) {
        cpp->destroy(instance);
    }
}

const char* ptx_field_name(const ptx_field_decl* field) {
    const auto* cpp = to_cpp(field);
    return cpp ? cpp->name : nullptr;
}

const char* ptx_field_description(const ptx_field_decl* field) {
    const auto* cpp = to_cpp(field);
    return cpp ? cpp->description : nullptr;
}

size_t ptx_field_size(const ptx_field_decl* field) {
    const auto* cpp = to_cpp(field);
    return cpp ? cpp->size : 0;
}

double ptx_field_min_value(const ptx_field_decl* field) {
    const auto* cpp = to_cpp(field);
    return cpp ? cpp->min_value : 0.0;
}

double ptx_field_max_value(const ptx_field_decl* field) {
    const auto* cpp = to_cpp(field);
    return cpp ? cpp->max_value : 0.0;
}

const void* ptx_field_type(const ptx_field_decl* field) {
    const auto* cpp = to_cpp(field);
    return cpp ? from_type(cpp->type) : nullptr;
}

void* ptx_field_get_ptr(const ptx_field_decl* field, void* object) {
    const auto* cpp = to_cpp(field);
    if (!cpp || !cpp->access.get_ptr) {
        return nullptr;
    }
    return cpp->access.get_ptr(object);
}

const void* ptx_field_get_const_ptr(const ptx_field_decl* field, const void* object) {
    const auto* cpp = to_cpp(field);
    if (!cpp || !cpp->access.get_cptr) {
        return nullptr;
    }
    return cpp->access.get_cptr(object);
}

const char* ptx_method_name(const ptx_method_desc* method) {
    const auto* cpp = to_cpp(method);
    return cpp ? cpp->name : nullptr;
}

const char* ptx_method_doc(const ptx_method_desc* method) {
    const auto* cpp = to_cpp(method);
    return cpp ? cpp->doc : nullptr;
}

bool ptx_method_is_static(const ptx_method_desc* method) {
    const auto* cpp = to_cpp(method);
    return cpp ? cpp->is_static : false;
}

size_t ptx_method_arg_count(const ptx_method_desc* method) {
    const auto* cpp = to_cpp(method);
    return cpp ? cpp->arg_types.count : 0;
}

const void* ptx_method_arg_type(const ptx_method_desc* method, size_t index) {
    const auto* cpp = to_cpp(method);
    if (!cpp || index >= cpp->arg_types.count) {
        return nullptr;
    }
    return from_type(cpp->arg_types.data[index]);
}

const void* ptx_method_return_type(const ptx_method_desc* method) {
    const auto* cpp = to_cpp(method);
    return cpp ? from_type(cpp->ret_type) : nullptr;
}

const char* ptx_method_signature(const ptx_method_desc* method) {
    const auto* cpp = to_cpp(method);
    return cpp ? cpp->signature : nullptr;
}

size_t ptx_method_return_size(const ptx_method_desc* method) {
    const auto* cpp = to_cpp(method);
    return cpp ? cpp->ret_size : 0;
}

void* ptx_method_invoke(const ptx_method_desc* method, void* instance, void** argv) {
    const auto* cpp = to_cpp(method);
    if (!cpp || !cpp->invoker) {
        return nullptr;
    }
    if (cpp->is_static) {
        return cpp->invoker(nullptr, argv);
    }
    return cpp->invoker(instance, argv);
}

void ptx_method_destroy_return(const ptx_method_desc* method, void* value) {
    const auto* cpp = to_cpp(method);
    if (cpp && cpp->destroy_ret && value) {
        cpp->destroy_ret(value);
    }
}

const char* ptx_constructor_signature(const ptx_constructor_desc* ctor) {
    const auto* cpp = to_cpp(ctor);
    return cpp ? cpp->signature : nullptr;
}

size_t ptx_constructor_arg_count(const ptx_constructor_desc* ctor) {
    const auto* cpp = to_cpp(ctor);
    return cpp ? cpp->arg_types.count : 0;
}

const void* ptx_constructor_arg_type(const ptx_constructor_desc* ctor, size_t index) {
    const auto* cpp = to_cpp(ctor);
    if (!cpp || index >= cpp->arg_types.count) {
        return nullptr;
    }
    return from_type(cpp->arg_types.data[index]);
}

void* ptx_constructor_invoke(const ptx_constructor_desc* ctor, void** argv) {
    const auto* cpp = to_cpp(ctor);
    if (!cpp || !cpp->invoker) {
        return nullptr;
    }
    return cpp->invoker(argv);
}

const char* ptx_type_info_name(const void* type_info) {
    const auto* ti = to_type(type_info);
    return ti ? ti->name() : nullptr;
}

const char* ptx_type_info_pretty_name(const void* type_info) {
    const auto* ti = to_type(type_info);
    if (!ti) {
        return nullptr;
    }
    g_pretty_name = ptx::Demangle(ti->name());
    return g_pretty_name.c_str();
}

size_t ptx_type_info_hash(const void* type_info) {
    const auto* ti = to_type(type_info);
    return ti ? ti->hash_code() : 0;
}

} // extern "C"
