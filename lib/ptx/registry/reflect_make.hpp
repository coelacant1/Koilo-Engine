#pragma once
#include <tuple>
#include <type_traits>
#include <utility>
#include "registry.hpp"

// Helpers to deref argv[] into typed values
namespace ptx::detail {

template <class T>
using arg_decay_t = std::remove_reference_t<T>;

template <class... Args, std::size_t... I>
auto ArgvAsTuple(void** argv, std::index_sequence<I...>) {
    return std::tuple< arg_decay_t<Args>&... >(
        *static_cast<arg_decay_t<Args>*>(argv[I])...
    );
}

template <class F, class Tuple, std::size_t... I>
auto ApplyTuple(F&& f, Tuple&& t, std::index_sequence<I...>) {
    return std::forward<F>(f)(std::get<I>(std::forward<Tuple>(t))...);
}

// Allocate return or swallow void
template <class Ret>
void* BoxReturn(Ret&& r) {
    if constexpr (std::is_void_v<std::remove_reference_t<Ret>>) {
        return nullptr;
    } else {
        using R = std::remove_cv_t<std::remove_reference_t<Ret>>;
        return new R(std::forward<Ret>(r));
    }
}

} // namespace ptx::detail

// Field maker (member pointer)
namespace ptx {

template <class C, class T>
inline FieldDecl MakeField(const char* name, T C::* member, const char* desc, float minV, float maxV) {
    return FieldDecl{
        name,
        &typeid(T),
        sizeof(T),
        FieldAccess{
            [member](void* obj) -> void* {
                return &(static_cast<C*>(obj)->*member);
            },
            [member](const void* obj) -> const void* {
                return &(static_cast<const C*>(obj)->*member);
            }
        },
        desc, minV, maxV
    };
}

// -------- Method makers (non-const member) --------
// non-const member
template <class C, class Ret, class... Args>
inline MethodDesc MakeMethod(const char* name, const char* desc, Ret (C::*pmf)(Args...)) {
    static const std::type_info* kParamTypes[] = { &typeid(Args)... };
    return MethodDesc{
        name, desc,
        std::is_void_v<Ret> ? nullptr : &typeid(Ret),
        kParamTypes, sizeof...(Args),
        [pmf](void* obj, void** argv) -> void* {
            auto* self = static_cast<C*>(obj);
            auto tup = ptx::detail::ArgvAsTuple<Args...>(argv, std::index_sequence_for<Args...>{});
            auto call = [self, pmf](auto&... a) -> Ret { return (self->*pmf)(a...); };
            if constexpr (std::is_void_v<Ret>) {
                ptx::detail::ApplyTuple(call, tup, std::index_sequence_for<Args...>{});
                return nullptr;
            } else {
                Ret r = ptx::detail::ApplyTuple(call, tup, std::index_sequence_for<Args...>{});
                return ptx::detail::BoxReturn(std::move(r));
            }
        }
    };
}

// const member
template <class C, class Ret, class... Args>
inline MethodDesc MakeMethod(const char* name, const char* desc, Ret (C::*pmf)(Args...) const) {
    static const std::type_info* kParamTypes[] = { &typeid(Args)... };
    return MethodDesc{
        name, desc,
        std::is_void_v<Ret> ? nullptr : &typeid(Ret),
        kParamTypes, sizeof...(Args),
        [pmf](void* obj, void** argv) -> void* {
            auto* self = static_cast<const C*>(obj);
            auto tup = ptx::detail::ArgvAsTuple<Args...>(argv, std::index_sequence_for<Args...>{});
            auto call = [self, pmf](auto&... a) -> Ret { return (self->*pmf)(a...); };
            if constexpr (std::is_void_v<Ret>) {
                ptx::detail::ApplyTuple(call, tup, std::index_sequence_for<Args...>{});
                return nullptr;
            } else {
                Ret r = ptx::detail::ApplyTuple(call, tup, std::index_sequence_for<Args...>{});
                return ptx::detail::BoxReturn(std::move(r));
            }
        }
    };
}

// static
template <class Ret, class... Args>
inline MethodDesc MakeSmethod(const char* name, const char* desc, Ret (*pf)(Args...)) {
    static const std::type_info* kParamTypes[] = { &typeid(Args)... };
    return MethodDesc{
        name, desc,
        std::is_void_v<Ret> ? nullptr : &typeid(Ret),
        kParamTypes, sizeof...(Args),
        [pf](void*, void** argv) -> void* {
            auto tup = ptx::detail::ArgvAsTuple<Args...>(argv, std::index_sequence_for<Args...>{});
            auto call = [pf](auto&... a) -> Ret { return pf(a...); };
            if constexpr (std::is_void_v<Ret>) {
                ptx::detail::ApplyTuple(call, tup, std::index_sequence_for<Args...>{});
                return nullptr;
            } else {
                Ret r = ptx::detail::ApplyTuple(call, tup, std::index_sequence_for<Args...>{});
                return ptx::detail::BoxReturn(std::move(r));
            }
        }
    };
}


} // namespace ptx
