#pragma once

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ptx_class_desc ptx_class_desc;
typedef struct ptx_field_decl ptx_field_decl;
typedef struct ptx_method_desc ptx_method_desc;
typedef struct ptx_constructor_desc ptx_constructor_desc;

size_t ptx_registry_class_count(void);
const ptx_class_desc* ptx_registry_class_at(size_t index);
const ptx_class_desc* ptx_registry_find_class(const char* name);

const char* ptx_class_name(const ptx_class_desc* cls);
size_t ptx_class_field_count(const ptx_class_desc* cls);
const ptx_field_decl* ptx_class_field_at(const ptx_class_desc* cls, size_t index);
const ptx_field_decl* ptx_class_find_field(const ptx_class_desc* cls, const char* name);
size_t ptx_class_method_count(const ptx_class_desc* cls);
const ptx_method_desc* ptx_class_method_at(const ptx_class_desc* cls, size_t index);
const ptx_method_desc* ptx_class_find_method(const ptx_class_desc* cls, const char* name);
size_t ptx_class_constructor_count(const ptx_class_desc* cls);
const ptx_constructor_desc* ptx_class_constructor_at(const ptx_class_desc* cls, size_t index);
void ptx_class_destroy_instance(const ptx_class_desc* cls, void* instance);

const char* ptx_field_name(const ptx_field_decl* field);
const char* ptx_field_description(const ptx_field_decl* field);
size_t ptx_field_size(const ptx_field_decl* field);
double ptx_field_min_value(const ptx_field_decl* field);
double ptx_field_max_value(const ptx_field_decl* field);
const void* ptx_field_type(const ptx_field_decl* field);
void* ptx_field_get_ptr(const ptx_field_decl* field, void* object);
const void* ptx_field_get_const_ptr(const ptx_field_decl* field, const void* object);

const char* ptx_method_name(const ptx_method_desc* method);
const char* ptx_method_doc(const ptx_method_desc* method);
bool ptx_method_is_static(const ptx_method_desc* method);
size_t ptx_method_arg_count(const ptx_method_desc* method);
const void* ptx_method_arg_type(const ptx_method_desc* method, size_t index);
const void* ptx_method_return_type(const ptx_method_desc* method);
const char* ptx_method_signature(const ptx_method_desc* method);
size_t ptx_method_return_size(const ptx_method_desc* method);
void* ptx_method_invoke(const ptx_method_desc* method, void* instance, void** argv);
void ptx_method_destroy_return(const ptx_method_desc* method, void* value);

const char* ptx_constructor_signature(const ptx_constructor_desc* ctor);
size_t ptx_constructor_arg_count(const ptx_constructor_desc* ctor);
const void* ptx_constructor_arg_type(const ptx_constructor_desc* ctor, size_t index);
void* ptx_constructor_invoke(const ptx_constructor_desc* ctor, void** argv);

const char* ptx_type_info_name(const void* type_info);
const char* ptx_type_info_pretty_name(const void* type_info);
size_t ptx_type_info_hash(const void* type_info);

#ifdef __cplusplus
}
#endif
