#pragma once

#include "../material/imaterial.hpp"
#include "../shader/ishader.hpp"

template<typename ParamBlock, typename ShaderT>
class MaterialT : public IMaterial, public ParamBlock {
    static const ShaderT& ShaderSingleton() {
        static const ShaderT Instance{};  // Created once, stored in flash
        return Instance;
    }
    static const IShader* ShaderPtr() { return &ShaderSingleton(); }

public:
    // Forward constructor args to ParamBlock aggregate‑initialiser
    template<typename... Args>
    explicit MaterialT(Args&&... ArgsPack)
        : IMaterial(ShaderPtr()),
          ParamBlock{std::forward<Args>(ArgsPack)...} {}

    // Override Update() when needed via derived ParamBlock + CRTP if desired
};
