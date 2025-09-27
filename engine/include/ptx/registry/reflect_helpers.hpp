#pragma once

#include <type_traits>
#include <utility>
#include <cstring>
#include "registry.hpp"
#include "demangle.hpp"

namespace ptx {

template<class T>
inline T& FieldRef(void* obj, const FieldDecl& fd) {
    return *static_cast<T*>(fd.access.get_ptr(obj));
}
template<class T>
inline const T& FieldCref(const void* obj, const FieldDecl& fd) {
    return *static_cast<const T*>(fd.access.get_cptr(obj));
}
template<class T>
inline void FieldSet(void* obj, const FieldDecl& fd, const T& v) {
    FieldRef<T>(obj, fd) = v;
}

template<class Ret>
inline Ret* InvokeRet(const MethodDesc& md, void* self, ...) { // caller uses: InvokeRet<T>(md, self, &a0, &a1, &a2, ...)
    void* argv_local[8];
    void** args = argv_local;

    return nullptr;
}

template<class Ret>
inline Ret* InvokeRet(const MethodDesc& md, void* self, void** argv) {
    void* p = md.is_static ? md.invoker(nullptr, argv) : md.invoker(self, argv);
    return static_cast<Ret*>(p);
}

inline const FieldDecl* FindField(const FieldList& fl, const char* name) {
    for (std::size_t i = 0; i < fl.count; ++i)
        if (std::strcmp(fl.data[i].name, name) == 0) return &fl.data[i];
    return nullptr;
}

inline const MethodDesc* FindMethod(const MethodList& ml, const char* name) {
    for (std::size_t i = 0; i < ml.count; ++i)
        if (std::strcmp(ml.data[i].name, name) == 0) return &ml.data[i];
    return nullptr;
}

inline std::string PrettyType(const std::type_info* ti) {
    return ti ? Demangle(ti->name()) : std::string("<null-type>");
}

inline std::string PrettySignature(const MethodDesc& m, const char* owner = nullptr) {
    std::string out;

    if (owner && *owner) {
        out += owner;
        out += "::";
    }

    out += m.name ? m.name : "<noname>";

    out += "(";
    for (size_t i = 0; i < m.arg_types.count; ++i) {
        if (i) out += ", ";
        out += PrettyType(m.arg_types.data[i]);
    }
    out += ")";

    out += " -> ";
    out += PrettyType(m.ret_type);

    if (m.is_static) out += " [static]";

    return out;
}

inline std::string PrettyCtor(const char* class_name, const ConstructorDesc& c) {
    std::string out;
    out += class_name ? class_name : "<Class>";
    out += "(";
    for (size_t i = 0; i < c.arg_types.count; ++i) {
        if (i) out += ", ";
        out += PrettyType(c.arg_types.data[i]);
    }
    out += ")";
    return out;
}

} // namespace ptx
