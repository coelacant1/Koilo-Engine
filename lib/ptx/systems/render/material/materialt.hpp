#pragma once

#include <utility>
#include "../material/imaterial.hpp"
#include "../shader/ishader.hpp"

/**
 * @file materialt.hpp
 * @brief Generic material boilerplate that binds a shader and inlines a param block.
 *
 * @tparam ParamBlock  POD/struct holding user parameters (mixed in via public inheritance).
 * @tparam ShaderT     Concrete shader type used by this material (must implement IShader).
 *
 * Pattern:
 * - Mixes @p ParamBlock into the material type (public inheritance) so its fields are directly
 *   accessible on the material instance.
 * - Binds a single process-wide @p ShaderT instance (singleton) to @ref IMaterial.
 * - Construction forwards any args to @p ParamBlock’s constructor.
 *
 * Override @ref IMaterial::Update(float) in a derived material type when you need animation.
 */
template<typename ParamBlock, typename ShaderT>
class MaterialT : public IMaterial, public ParamBlock {
    /** @brief Process-wide shader singleton. */
    static const ShaderT& ShaderSingleton() {
        static const ShaderT Instance{};  // Created once
        return Instance;
    }

    /** @brief Non-owning pointer to the shader singleton for IMaterial base. */
    static const IShader* ShaderPtr() { return &ShaderSingleton(); }

public:
    /**
     * @brief Construct and bind shader; forward args to ParamBlock.
     * @tparam Args      Parameter pack types forwarded to ParamBlock.
     * @param  ArgsPack  Arguments forwarded to ParamBlock’s constructor.
     */
    template<typename... Args>
    explicit MaterialT(Args&&... ArgsPack)
        : IMaterial(ShaderPtr()),
          ParamBlock(std::forward<Args>(ArgsPack)...) {}
};
