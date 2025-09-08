#pragma once
#include "registry.hpp"

namespace ptx {

// CRTP: only declarations, derived types can define their own static functions with the same signatures
template<class Derived>
struct Reflectable {
    static ptx::FieldList  Fields();
    static ptx::MethodList Methods();
};

} // namespace ptx
