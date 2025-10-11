#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "../../../../registry/reflect_macros.hpp"

#include "../../material/imaterial.hpp"
#include "../../../../core/math/vector3d.hpp"
#include "../../../../core/color/rgbcolor.hpp"

/**
 * @file combineparams.hpp
 * @brief Runtime-sized parameter block for a composite material with blendable layers.
 *
 * Stores per-layer blend method, material pointer, and opacity using dynamic storage.
 * Helpers manage adding/editing layers and exposing counts for runtime-driven UIs.
 */

/**
 * @brief Parameters for a composite material stack.
 */
struct CombineParams {
    /**
     * @brief Per-layer blend operation.
     */
    enum class Method : uint8_t {
        Base,           ///< Use as base layer (typically full-opacity)
        Add,            ///< Additive blend
        Subtract,       ///< Subtractive blend
        Multiply,       ///< Multiplicative blend
        Divide,         ///< Division blend
        Darken,         ///< Min-like blend
        Lighten,        ///< Max-like blend
        Screen,         ///< Screen blend
        Overlay,        ///< Overlay blend
        SoftLight,      ///< Soft light blend
        Replace,        ///< Replace destination with source
        EfficientMask,  ///< Masking optimized for performance
        Bypass          ///< Skip layer (no-op)
    };

    std::vector<Method>           methods{};    ///< Blend method per layer
    std::vector<const IMaterial*> materials{};  ///< Non-owning material pointers
    std::vector<float>            opacities{};  ///< Opacity per layer [0..1]

    CombineParams() = default;

    explicit CombineParams(std::size_t reserveCount) {
        Reserve(reserveCount);
    }

    /**
     * @brief Append a layer if capacity permits.
     * @param m   Blend method.
     * @param mat Non-owning material pointer.
     * @param a   Opacity [0..1].
     */
    void AddLayer(Method m, const IMaterial* mat, float a) {
        methods.push_back(m);
        materials.push_back(mat);
        opacities.push_back(a);
    }

    /**
     * @brief Set blend method of an existing layer.
     * @param i Layer index (< count).
     * @param m New method.
     */
    void SetMethod(std::size_t i, Method m)  { if (i < methods.size()) methods[i] = m; }

    /**
     * @brief Set opacity of an existing layer.
     * @param i Layer index (< count).
     * @param a New opacity [0..1].
     */
    void SetOpacity(std::size_t i, float a)  { if (i < opacities.size()) opacities[i] = a; }

    /**
     * @brief Set material pointer of an existing layer.
     * @param i   Layer index (< count).
     * @param mat New non-owning material pointer.
     */
    void SetMaterial(std::size_t i, const IMaterial* mat) {
        if (i < materials.size()) materials[i] = mat;
    }

    /**
     * @brief Remove all layers.
     */
    void Clear() {
        methods.clear();
        materials.clear();
        opacities.clear();
    }

    /**
     * @brief Ensure storage for at least @p count layers without resizing vectors.
     */
    void Reserve(std::size_t count) {
        methods.reserve(count);
        materials.reserve(count);
        opacities.reserve(count);
    }

    /** @brief Number of active layers. */
    [[nodiscard]] std::size_t LayerCount() const noexcept { return methods.size(); }

    PTX_BEGIN_FIELDS(CombineParams)
        PTX_FIELD(CombineParams, methods, "Methods", 0, 0),
        PTX_FIELD(CombineParams, materials, "Materials", 0, 0),
        PTX_FIELD(CombineParams, opacities, "Opacities", __FLT_MIN__, __FLT_MAX__)
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(CombineParams)
        PTX_METHOD_AUTO(CombineParams, AddLayer, "Add layer"),
        PTX_METHOD_AUTO(CombineParams, SetMethod, "Set method"),
        PTX_METHOD_AUTO(CombineParams, SetOpacity, "Set opacity"),
        PTX_METHOD_AUTO(CombineParams, SetMaterial, "Set material"),
        PTX_METHOD_AUTO(CombineParams, Clear, "Clear"),
        PTX_METHOD_AUTO(CombineParams, Reserve, "Reserve"),
        PTX_METHOD_AUTO(CombineParams, LayerCount, "Layer count")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(CombineParams)
        PTX_CTOR0(CombineParams),
        PTX_CTOR(CombineParams, std::size_t)
    PTX_END_DESCRIBE(CombineParams)

};
