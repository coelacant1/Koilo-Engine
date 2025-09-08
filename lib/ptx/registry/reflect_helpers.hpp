#pragma once
#include <type_traits>
#include "registry.hpp"

// Field helpers
template<class T>
inline T& FieldRef(void* obj, const ptx::FieldDecl& f) {
    return *static_cast<T*>(f.access.get_ptr(obj));
}
template<class T>
inline const T& FieldCref(const void* obj, const ptx::FieldDecl& f) {
    return *static_cast<const T*>(f.access.get_cptr(obj));
}
template<class T>
inline void FieldSet(void* obj, const ptx::FieldDecl& f, const T& v) {
    FieldRef<T>(obj, f) = v;
}

// Method helpers
template<class Ret=void, class... Args>
inline Ret* InvokeRet(const ptx::MethodDesc& m, void* obj, Args*... args) {
    void* argv[] = { static_cast<void*>(args)... };

    return static_cast<Ret*>(m.invoker(obj, argv));
}

template<class... Args>
inline void InvokeVoid(const ptx::MethodDesc& m, void* obj, Args*... args) {
    void* argv[] = { static_cast<void*>(args)... };
    m.invoker(obj, argv); // returns nullptr
}
