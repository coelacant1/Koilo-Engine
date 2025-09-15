#pragma once

#include <cstddef>
#include <cstdint>
#include "../../../../registry/reflect_macros.hpp"

#include "../imaterial.hpp"
#include "../materialt.hpp"
#include "../../shader/ishader.hpp"

#include "../../shader/implementations/combineparams.hpp"
#include "../../shader/implementations/combineshader.hpp"

/**
 * @file combinematerial.hpp
 * @brief Material wrapper over CombineParams/CombineShader using MaterialT glue.
 * @tparam N Maximum number of layers.
 */
template <size_t N>
class CombineMaterial : public MaterialT<CombineParams<N>, CombineShaderT<N>> {
    using Base = MaterialT<CombineParams<N>, CombineShaderT<N>>;

public:
    using Method = typename CombineParams<N>::Method;
    using Base::Base;
    CombineMaterial() = default;

    /**
     * @brief Append a new material layer.
     * @param method   Blending method for the new layer.
     * @param material Non-owning pointer to the material.
     * @param opacity  Initial opacity [0..1].
     */
    void AddMaterial(Method method, const IMaterial* material, float opacity) {
        this->template CombineParams<N>::Add(method, material, opacity);
    }

    /**
     * @brief Set the blend method of an existing layer.
     * @param index  Layer index.
     * @param method New method.
     */
    void SetMethod(uint8_t index, Method method) {
        this->template CombineParams<N>::SetMethod(index, method);
    }

    /**
     * @brief Set the opacity of an existing layer.
     * @param index   Layer index.
     * @param opacity Opacity [0..1].
     */
    void SetOpacity(uint8_t index, float opacity) {
        this->template CombineParams<N>::SetOpacity(index, opacity);
    }

    /**
     * @brief Replace the material pointer of an existing layer.
     * @param index    Layer index.
     * @param material New non-owning material pointer.
     */
    void SetMaterial(uint8_t index, const IMaterial* material) {
        this->template CombineParams<N>::SetMaterial(index, material);
    }

    /* NOTE: CombineMaterial is a template; verify macros accept template types. */
    PTX_BEGIN_FIELDS(CombineMaterial)
        /* TODO: PTX_FIELD(CombineMaterial, member, "Doc", min, max) */
    PTX_END_FIELDS
    
    PTX_BEGIN_METHODS(CombineMaterial)
        PTX_METHOD_AUTO(CombineMaterial, AddMaterial, "Add material"),
        PTX_METHOD_AUTO(CombineMaterial, SetMethod, "Set method"),
        PTX_METHOD_AUTO(CombineMaterial, SetOpacity, "Set opacity"),
        PTX_METHOD_AUTO(CombineMaterial, SetMaterial, "Set material")
    PTX_END_METHODS
    
    PTX_BEGIN_DESCRIBE(CombineMaterial)
        PTX_CTOR0(CombineMaterial)
    PTX_END_DESCRIBE(CombineMaterial)
    
};
