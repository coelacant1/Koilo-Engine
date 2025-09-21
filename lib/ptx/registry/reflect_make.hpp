#pragma once

#include <tuple>
#include <type_traits>
#include <utility>
#include <functional>
#include <memory>
#include "registry.hpp"

namespace ptx::detail {

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

} // namespace ptx::detail

namespace ptx::make {

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

template<class R, class... A>
struct FnSig<R (*)(A...)> { // free/static function pointer
    using RetT = R;
    using ArgsTuple = std::tuple<A...>;

    static constexpr size_t Arity = sizeof...(A);
};


template<class Tuple, std::size_t... I>
inline ptx::TypeSpan MakeTypeSpanFromTuple(std::index_sequence<I...>) {
    static const std::type_info* arr[] = { &typeid(std::tuple_element_t<I, Tuple>)... };

    return { arr, sizeof...(I) };
}

template<class Tuple, std::size_t... I>
inline ptx::TypeSpan MakeTypeSpan_Tuple(std::index_sequence<I...>) {
    static const std::type_info* arr[] = { &typeid(std::tuple_element_t<I, Tuple>)... };

    return { arr, sizeof...(I) };
}

template<class Tuple>
inline ptx::TypeSpan MakeTypeSpan_Tuple() {
    return MakeTypeSpan_Tuple<Tuple>(std::make_index_sequence<std::tuple_size_v<Tuple>>{});
}

template<class... Ts>
inline ptx::TypeSpan MakeTypeSpan_Pack() {
    static const std::type_info* arr[] = { &typeid(Ts)... };

    return { arr, sizeof...(Ts) };
}

template<class C, auto PMF>
void* InstInvoke(void* self, void** argv) { // Member function thunk
    using S   = ptx::make::FnSig<decltype(PMF)>;
    using Ret = typename S::RetT;
    using AT  = typename S::ArgsTuple;

    constexpr auto N = S::Arity;

    auto obj = static_cast<C*>(self);
    auto tup = ptx::detail::ArgvAsTuple<AT>(argv, std::make_index_sequence<N>{});
    auto call = [obj](auto&... a) -> Ret { return (obj->*PMF)(a...); };

    if constexpr (std::is_void_v<Ret>) {
        ptx::detail::ApplyTuple(call, tup, std::make_index_sequence<N>{});

        return nullptr;
    } else {
        auto&& result = ptx::detail::ApplyTuple(call, tup, std::make_index_sequence<N>{});

        return ptx::detail::BoxReturn(std::forward<decltype(result)>(result));
    }
}


template<auto PF>
void* StaticInvoke(void*, void** argv) { // Static / free function thunk
    using S   = ptx::make::FnSig<decltype(PF)>;
    using Ret = typename S::RetT;
    using AT  = typename S::ArgsTuple;

    constexpr auto N = S::Arity;

    auto tup = ptx::detail::ArgvAsTuple<AT>(argv, std::make_index_sequence<N>{});
    auto call = [](auto&... a) -> Ret { return PF(a...); };

    if constexpr (std::is_void_v<Ret>) {
        ptx::detail::ApplyTuple(call, tup, std::make_index_sequence<N>{});

        return nullptr;
    } else {
        auto&& result = ptx::detail::ApplyTuple(call, tup, std::make_index_sequence<N>{});

        return ptx::detail::BoxReturn(std::forward<decltype(result)>(result));
    }
}

template<class C, auto PMF>
ptx::MethodDesc MakeMethod(const char* name, const char* doc) { // Member function
    using S   = FnSig<decltype(PMF)>;
    using Ret = typename S::RetT;
    using AT  = typename S::ArgsTuple;

    return ptx::MethodDesc{
        /* name       */ name,
        /* doc        */ doc,
        /* ret_type   */ &typeid(Ret),
        /* arg_types  */ MakeTypeSpan_Tuple<AT>(),
        /* argc       */ std::tuple_size_v<AT>,
        /* is_static  */ false,
        /* invoker    */ &ptx::make::InstInvoke<C, PMF>,
        /* signature  */ nullptr,
        /*ret_size     */ ptx::detail::ReturnTraits<Ret>::Size,
        /*destroy_ret  */ ptx::detail::ReturnTraits<Ret>::Destroy
    };
}

template<auto PF>
ptx::MethodDesc MakeSmethod(const char* name, const char* doc) { // Static / free function
    using S   = FnSig<decltype(PF)>;
    using Ret = typename S::RetT;
    using AT  = typename S::ArgsTuple;

    return ptx::MethodDesc{
        /* name       */ name,
        /* doc        */ doc,
        /* ret_type   */ &typeid(Ret),
        /* arg_types  */ MakeTypeSpan_Tuple<AT>(),
        /* argc       */ std::tuple_size_v<AT>,
        /* is_static  */ true,
        /* invoker    */ &ptx::make::StaticInvoke<PF>,
        /* signature  */ nullptr,
        /*ret_size     */ ptx::detail::ReturnTraits<Ret>::Size,
        /*destroy_ret  */ ptx::detail::ReturnTraits<Ret>::Destroy
    };
}

template<class T, class... A>
struct CtorThunk {
    static void* Create(void** argv) { // Constructor thunk
        auto tup = ptx::detail::ArgvAsTuple<A...>(argv, std::index_sequence_for<A...>{});
        auto call = [](auto&... a) { return new T(a...); };

        return ptx::detail::ApplyTuple(call, tup, std::index_sequence_for<A...>{});
    }
};

template<class C, class... A>
ptx::ConstructorDesc MakeCtor(const char* pretty = nullptr) {
    struct Thunk {
        static void* Create(void** argv) {
            auto tup  = ptx::detail::ArgvAsTuple<std::tuple<A...>>(argv, std::index_sequence_for<A...>{});
            auto call = [](auto&... a) { return new C(a...); };

            return ptx::detail::ApplyTuple(call, tup, std::index_sequence_for<A...>{});
        }
    };
    static const ptx::TypeSpan ts = ptx::MakeTypeSpan<A...>();
    return ptx::ConstructorDesc{ ts, pretty, &Thunk::Create };
}

} // namespace ptx::make
