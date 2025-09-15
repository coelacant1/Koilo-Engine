#pragma once

#include "../ishader.hpp" // for IMaterial forward usage via pointers
#include "../../../../core/math/vector3d.hpp"
#include "../../../../core/color/rgbcolor.hpp"
#include <cstdint>
#include <algorithm>
#include "../../../../registry/reflect_macros.hpp"

/**
 * @file combineparams.hpp
 * @brief Parameter block for a composite material with N layers.
 *
 * Stores per-layer blend method, material pointer, and opacity, with small helpers
 * for adding and editing layers. Intended to be paired with a shader that interprets
 * these parameters at shade time.
 *
 * @tparam N Maximum number of layers.
 */

/**
 * @tparam N Maximum number of layers.
 * @brief Parameters for a composite material stack.
 */
template <size_t N>
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

    Method                 method[N]      {};        ///< Blend method per layer
    const IMaterial*       materials[N]   { nullptr };///< Non-owning material pointers
    float                  opacity[N]     { 0.0f };  ///< Opacity per layer [0..1]
    uint8_t                count          = 0;       ///< Number of active layers

    /**
     * @brief Append a layer if capacity permits.
     * @param m   Blend method.
     * @param mat Non-owning material pointer.
     * @param a   Opacity [0..1].
     */
    void Add(Method m, const IMaterial* mat, float a) {
        if (count < N) {
            method[count]    = m;
            materials[count] = mat;
            opacity[count]   = a;
            ++count;
        }
    }

    /**
     * @brief Set blend method of an existing layer.
     * @param i Layer index (< count).
     * @param m New method.
     */
    void SetMethod(uint8_t i, Method m)  { if (i < count) method[i] = m; }

    /**
     * @brief Set opacity of an existing layer.
     * @param i Layer index (< count).
     * @param a New opacity [0..1].
     */
    void SetOpacity(uint8_t i, float a)  { if (i < count) opacity[i] = a; }

    /**
     * @brief Set material pointer of an existing layer.
     * @param i   Layer index (< count).
     * @param mat New non-owning material pointer.
     */
    void SetMaterial(uint8_t i, const IMaterial* mat) { if (i < count) materials[i] = mat; }

    /* NOTE: CombineParams is a template; verify macros accept template types. */
    PTX_BEGIN_FIELDS(CombineParams)
        PTX_FIELD(CombineParams, method, "Method", 0, 0),
        PTX_FIELD(CombineParams, materials, "Materials", 0, 0),
        PTX_FIELD(CombineParams, opacity, "Opacity", 0, 0),
        PTX_FIELD(CombineParams, count, "Count", 0, 255)
    PTX_END_FIELDS
    
    PTX_BEGIN_METHODS(CombineParams)
        PTX_METHOD_AUTO(CombineParams, Add, "Add"),
        PTX_METHOD_AUTO(CombineParams, SetMethod, "Set method"),
        PTX_METHOD_AUTO(CombineParams, SetOpacity, "Set opacity"),
        PTX_METHOD_AUTO(CombineParams, SetMaterial, "Set material")
    PTX_END_METHODS
    
    PTX_BEGIN_DESCRIBE(CombineParams)
        /* TODO: PTX_CTOR0(CombineParams) or PTX_CTOR(CombineParams, ...) */
    PTX_END_DESCRIBE(CombineParams)
    
};
