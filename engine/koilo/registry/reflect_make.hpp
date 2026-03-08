// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <tuple>
#include <type_traits>
#include <utility>
#include <functional>
#include "registry.hpp"

namespace koilo::detail {

template<typename T>
struct ReturnTraits {
    using Storage = std::remove_cv_t<std::remove_reference_t<T>>;
    static constexpr bool IsVoid = std::is_void_v<Storage>;
    static constexpr bool IsPointer = std::is_pointer_v<Storage>;
    static constexpr bool IsReference = std::is_reference_v<T>;
    static constexpr bool Owns = !IsVoid && !IsPointer && !IsReference;
    static constexpr std::size_t Size = []() constexpr {
        if constexpr (IsVoid) {
            return std::size_t(0);
        } else {
            return sizeof(Storage);
        }
    }();
    using DestroyFn = void(*)(void*);
    static constexpr DestroyFn Destroy = []() constexpr -> DestroyFn {
        if constexpr (Owns) {
            return +[](void* p) {
                delete static_cast<Storage*>(p);
            };
        } else {
            return nullptr;
        }
    }();
};

template<>
struct ReturnTraits<void> {
    using Storage = void;
    static constexpr bool IsVoid = true;
    static constexpr bool IsPointer = false;
    static constexpr bool IsReference = false;
    static constexpr bool Owns = false;
    static constexpr std::size_t Size = std::size_t(0);
    using DestroyFn = void(*)(void*);
    static constexpr DestroyFn Destroy = nullptr;
};


template<class F, class Tuple, std::size_t... I>
decltype(auto) ApplyTuple(F&& f, Tuple&& t, std::index_sequence<I...>) {
    return std::invoke(std::forward<F>(f),
                       std::get<I>(std::forward<Tuple>(t))...);
}

template<class Tuple, std::size_t... I>
auto ArgvAsTuple(void** argv, std::index_sequence<I...>) {
    using Tup = std::tuple<
        std::add_lvalue_reference_t<std::tuple_element_t<I, Tuple>>...
    >;
    return Tup(
        *static_cast<std::remove_reference_t<std::tuple_element_t<I, Tuple>>*>(argv[I])...
    );
}

template<class T>
void* BoxReturn(T&& v) {
    using Traits = ReturnTraits<T>;
    using Storage = typename Traits::Storage;

    if constexpr (Traits::IsVoid) {
        return nullptr;
    } else if constexpr (Traits::IsReference) {
        return const_cast<void*>(static_cast<const void*>(std::addressof(v)));
    } else if constexpr (Traits::IsPointer) {
        return const_cast<void*>(static_cast<const void*>(v));
    } else {
        return static_cast<void*>(new Storage(std::forward<T>(v)));
    }
}

} // namespace koilo::detail

namespace koilo::make {

template<class> struct FnSig;

template<class C, class R, class... A>
struct FnSig<R (C::*)(A...)> { // member
    using RetT = R;
    using ArgsTuple = std::tuple<A...>;

    static constexpr size_t Arity = sizeof...(A);
};

template<class C, class R, class... A>
struct FnSig<R (C::*)(A...) const> { // const member
    using RetT = R;
    using ArgsTuple = std::tuple<A...>;

    static constexpr size_t Arity = sizeof...(A);
};

template<class C, class R, class... A>
struct FnSig<R (C::*)(A...) noexcept> { // noexcept member
    using RetT = R;
    using ArgsTuple = std::tuple<A...>;

    static constexpr size_t Arity = sizeof...(A);
};

template<class C, class R, class... A>
struct FnSig<R (C::*)(A...) const noexcept> { // const noexcept member
    using RetT = R;
    using ArgsTuple = std::tuple<A...>;

    static constexpr size_t Arity = sizeof...(A);
};

template<class R, class... A>
struct FnSig<R (*)(A...)> { // free/static function pointer
    using RetT = R;
    using ArgsTuple = std::tuple<A...>;

    static constexpr size_t Arity = sizeof...(A);
};

template<class R, class... A>
struct FnSig<R (*)(A...) noexcept> { // free/static noexcept function pointer
    using RetT = R;
    using ArgsTuple = std::tuple<A...>;

    static constexpr size_t Arity = sizeof...(A);
};


template<class Tuple, std::size_t... I>
inline koilo::TypeSpan MakeTypeSpanFromTuple(std::index_sequence<I...>) {
#if KL_HAS_RTTI
    static const std::type_info* arr[] = { &typeid(std::tuple_element_t<I, Tuple>)... };
    return { arr, sizeof...(I) };
#else
    return { nullptr, 0 };
#endif
}

template<class Tuple, std::size_t... I>
inline koilo::TypeSpan MakeTypeSpan_Tuple(std::index_sequence<I...>) {
#if KL_HAS_RTTI
    static const std::type_info* arr[] = { &typeid(std::tuple_element_t<I, Tuple>)... };
    return { arr, sizeof...(I) };
#else
    return { nullptr, 0 };
#endif
}

template<class Tuple>
inline koilo::TypeSpan MakeTypeSpan_Tuple() {
    return MakeTypeSpan_Tuple<Tuple>(std::make_index_sequence<std::tuple_size_v<Tuple>>{});
}

template<class... Ts>
inline koilo::TypeSpan MakeTypeSpan_Pack() {
#if KL_HAS_RTTI
    static const std::type_info* arr[] = { &typeid(Ts)... };
    return { arr, sizeof...(Ts) };
#else
    return { nullptr, 0 };
#endif
}

template<class C, auto PMF>
void* InstInvoke(void* self, void** argv) { // Member function thunk
    using S   = koilo::make::FnSig<decltype(PMF)>;
    using Ret = typename S::RetT;
    using AT  = typename S::ArgsTuple;

    constexpr auto N = S::Arity;

    auto obj = static_cast<C*>(self);
    auto tup = koilo::detail::ArgvAsTuple<AT>(argv, std::make_index_sequence<N>{});
    auto call = [obj](auto&... a) -> Ret { return (obj->*PMF)(a...); };

    if constexpr (std::is_void_v<Ret>) {
        koilo::detail::ApplyTuple(call, tup, std::make_index_sequence<N>{});

        return nullptr;
    } else {
        auto&& result = koilo::detail::ApplyTuple(call, tup, std::make_index_sequence<N>{});

        return koilo::detail::BoxReturn(std::forward<decltype(result)>(result));
    }
}


template<auto PF>
void* StaticInvoke(void*, void** argv) { // Static / free function thunk
    using S   = koilo::make::FnSig<decltype(PF)>;
    using Ret = typename S::RetT;
    using AT  = typename S::ArgsTuple;

    constexpr auto N = S::Arity;

    auto tup = koilo::detail::ArgvAsTuple<AT>(argv, std::make_index_sequence<N>{});
    auto call = [](auto&... a) -> Ret { return PF(a...); };

    if constexpr (std::is_void_v<Ret>) {
        koilo::detail::ApplyTuple(call, tup, std::make_index_sequence<N>{});

        return nullptr;
    } else {
        auto&& result = koilo::detail::ApplyTuple(call, tup, std::make_index_sequence<N>{});

        return koilo::detail::BoxReturn(std::forward<decltype(result)>(result));
    }
}

template<class C, auto PMF>
koilo::MethodDesc MakeMethod(const char* name, const char* doc) { // Member function
    using S   = FnSig<decltype(PMF)>;
    using Ret = typename S::RetT;
    using AT  = typename S::ArgsTuple;

    return koilo::MethodDesc{
        /* name       */ name,
        /* doc        */ doc,
#if KL_HAS_RTTI
        /* ret_type   */ &typeid(Ret),
#else
        /* ret_type   */ nullptr,
#endif
        /* arg_types  */ MakeTypeSpan_Tuple<AT>(),
        /* argc       */ std::tuple_size_v<AT>,
        /* is_static  */ false,
        /* invoker    */ &koilo::make::InstInvoke<C, PMF>,
        /* signature  */ nullptr,
        /*ret_size     */ koilo::detail::ReturnTraits<Ret>::Size,
        /*destroy_ret  */ koilo::detail::ReturnTraits<Ret>::Destroy,
        /*ret_is_pointer*/ koilo::detail::ReturnTraits<Ret>::IsPointer,
        /*ret_owns     */ koilo::detail::ReturnTraits<Ret>::Owns
    };
}

template<auto PF>
koilo::MethodDesc MakeSmethod(const char* name, const char* doc) { // Static / free function
    using S   = FnSig<decltype(PF)>;
    using Ret = typename S::RetT;
    using AT  = typename S::ArgsTuple;

    return koilo::MethodDesc{
        /* name       */ name,
        /* doc        */ doc,
#if KL_HAS_RTTI
        /* ret_type   */ &typeid(Ret),
#else
        /* ret_type   */ nullptr,
#endif
        /* arg_types  */ MakeTypeSpan_Tuple<AT>(),
        /* argc       */ std::tuple_size_v<AT>,
        /* is_static  */ true,
        /* invoker    */ &koilo::make::StaticInvoke<PF>,
        /* signature  */ nullptr,
        /*ret_size     */ koilo::detail::ReturnTraits<Ret>::Size,
        /*destroy_ret  */ koilo::detail::ReturnTraits<Ret>::Destroy,
        /*ret_is_pointer*/ koilo::detail::ReturnTraits<Ret>::IsPointer,
        /*ret_owns     */ koilo::detail::ReturnTraits<Ret>::Owns
    };
}

template<class T, class... A>
struct CtorThunk {
    static void* Create(void** argv) { // Constructor thunk
        auto tup = koilo::detail::ArgvAsTuple<A...>(argv, std::index_sequence_for<A...>{});
        auto call = [](auto&... a) { return new T(a...); };

        return koilo::detail::ApplyTuple(call, tup, std::index_sequence_for<A...>{});
    }
};

template<class C, class... A>
koilo::ConstructorDesc MakeCtor(const char* pretty = nullptr) {
    struct Thunk {
        static void* Create(void** argv) {
            auto tup  = koilo::detail::ArgvAsTuple<std::tuple<A...>>(argv, std::index_sequence_for<A...>{});
            auto call = [](auto&... a) { return new C(a...); };

            return koilo::detail::ApplyTuple(call, tup, std::index_sequence_for<A...>{});
        }
    };
    static const koilo::TypeSpan ts = koilo::MakeTypeSpan<A...>();
    return koilo::ConstructorDesc{ ts, pretty, &Thunk::Create, sizeof...(A) };
}

// MakeCtor with default-parameter support: min_argc < total argc.
// The invoker is generated for the FULL arg list; the caller must
// zero-fill args between actual and max before calling.
template<size_t MinArgc, class C, class... A>
koilo::ConstructorDesc MakeCtorDefaults(const char* pretty = nullptr) {
    static_assert(MinArgc <= sizeof...(A), "min_argc must be <= total args");
    struct Thunk {
        static void* Create(void** argv) {
            auto tup  = koilo::detail::ArgvAsTuple<std::tuple<A...>>(argv, std::index_sequence_for<A...>{});
            auto call = [](auto&... a) { return new C(a...); };
            return koilo::detail::ApplyTuple(call, tup, std::index_sequence_for<A...>{});
        }
    };
    static const koilo::TypeSpan ts = koilo::MakeTypeSpan<A...>();
    return koilo::ConstructorDesc{ ts, pretty, &Thunk::Create, MinArgc };
}

} // namespace koilo::make
