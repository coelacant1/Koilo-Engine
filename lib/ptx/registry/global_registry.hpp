#pragma once

#include <vector>
#include "registry.hpp"

namespace ptx {
inline std::vector<const ClassDesc*>& GlobalClassRegistry() {
    static std::vector<const ClassDesc*> v;

    return v;
}

struct AutoRegistrar {
    explicit AutoRegistrar(const ClassDesc* cd) { GlobalClassRegistry().push_back(cd); }
};
}
