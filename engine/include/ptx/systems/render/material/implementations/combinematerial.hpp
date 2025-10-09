#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "../imaterial.hpp"
#include "../materialt.hpp"
#include "../../shader/ishader.hpp"

#include "../../shader/implementations/combineparams.hpp"
#include "../../shader/implementations/combineshader.hpp"
#include "../../../../registry/reflect_macros.hpp"

/**
 * @file combinematerial.hpp
 * @brief Runtime material wrapper over CombineParams/CombineShader using MaterialT glue.
 */
class CombineMaterial : public MaterialT<CombineParams, CombineShader> {
public:
    using Method = CombineParams::Method;

    explicit CombineMaterial(std::size_t reserveCount = 4)
        : MaterialT<CombineParams, CombineShader>(reserveCount) {}

    /**
     * @brief Append a new material layer.
     * @param method   Blending method for the new layer.
     * @param material Non-owning pointer to the material.
     * @param opacity  Initial opacity [0..1].
     */
    void AddMaterial(Method method, const IMaterial* material, float opacity) {
        CombineParams::AddLayer(method, material, opacity);
    }

    /**
     * @brief Set the blend method of an existing layer.
     * @param index  Layer index.
     * @param method New method.
     */
    void SetMethod(uint8_t index, Method method) {
        CombineParams::SetMethod(index, method);
    }

    /**
     * @brief Set the opacity of an existing layer.
     * @param index   Layer index.
     * @param opacity Opacity [0..1].
     */
    void SetOpacity(uint8_t index, float opacity) {
        CombineParams::SetOpacity(index, opacity);
    }

    /**
     * @brief Replace the material pointer of an existing layer.
     * @param index    Layer index.
     * @param material New non-owning material pointer.
     */
    void SetMaterial(uint8_t index, const IMaterial* material) {
        CombineParams::SetMaterial(index, material);
    }

    /** @brief Remove all layers. */
    void ClearLayers() { CombineParams::Clear(); }

    /** @brief Reserve backing storage for future layers. */
    void ReserveLayers(std::size_t count) { CombineParams::Reserve(count); }

    /** @brief Query number of active layers. */
    [[nodiscard]] std::size_t LayerCount() const { return CombineParams::LayerCount(); }

    PTX_BEGIN_FIELDS(CombineMaterial)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(CombineMaterial)
        PTX_METHOD_AUTO(CombineMaterial, AddMaterial, "Add material"),
        PTX_METHOD_AUTO(CombineMaterial, SetMethod, "Set method"),
        PTX_METHOD_AUTO(CombineMaterial, SetOpacity, "Set opacity"),
        PTX_METHOD_AUTO(CombineMaterial, SetMaterial, "Set material"),
        PTX_METHOD_AUTO(CombineMaterial, ClearLayers, "Clear layers"),
        PTX_METHOD_AUTO(CombineMaterial, ReserveLayers, "Reserve layers"),
        PTX_METHOD_AUTO(CombineMaterial, LayerCount, "Layer count")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(CombineMaterial)
        PTX_CTOR(CombineMaterial, std::size_t)
    PTX_END_DESCRIBE(CombineMaterial)
};
