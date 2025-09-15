#pragma once

#include "../imaterial.hpp"
#include "../materialt.hpp"
#include "../../shader/ishader.hpp"
#include "../../../../registry/reflect_macros.hpp"

#include "../../shader/implementations/materialmaskparams.hpp"
#include "../../shader/implementations/materialmaskshader.hpp"

/**
 * @file materialmask.hpp
 * @brief Material wrapper that blends two materials using a mask Shape with adjustable opacity.
 */

/**
 * @class MaterialMask
 * @brief Masks between an inner (shape) material and an outer material using a Shape.
 *
 * Combines @ref MaterialMaskParams and @ref MaterialMaskShader via @ref MaterialT.
 * The mask geometry is provided by @ref Shape, while @ref materialShape and
 * @ref materialOuter define the materials to be combined. Opacity controls the blend strength.
 */
class MaterialMask : public MaterialT<MaterialMaskParams, MaterialMaskShader> {
    using Base = MaterialT<MaterialMaskParams, MaterialMaskShader>;
public:
    using Base::Base; ///< Inherit aggregate constructor if desired.

    /**
     * @brief Construct and bind shape, inner material, and outer material.
     * @param materialShape Material used inside/where the shape mask applies (non-owning).
     * @param materialOuter Material used outside the shape mask (non-owning).
     * @param shape         Mask geometry (non-owning).
     */
    MaterialMask(const IMaterial* materialShape,
                 const IMaterial* materialOuter,
                 Shape*           shape)
    {
        this->materialShape = materialShape;
        this->materialOuter = materialOuter;
        this->shape         = shape;
        this->opacity       = 1.0f;
    }

    /** @brief Set global opacity for the mask blend. */
    void SetOpacity(float o)            { this->opacity = o; }

    /** @brief Get a writable reference to opacity for animation or binding. */
    float* GetOpacityReference()        { return &this->opacity; }

    /** @brief Set the mask Shape pointer. */
    void SetShape(Shape* s)                         { this->shape = s; }

    /** @brief Set the inner/material used where the mask applies. */
    void SetMaterialShape(const IMaterial* m)       { this->materialShape = m; }

    /** @brief Set the outer/material used where the mask does not apply. */
    void SetMaterialOuter(const IMaterial* m)       { this->materialOuter = m; }

    PTX_BEGIN_FIELDS(MaterialMask)
        /* TODO: PTX_FIELD(MaterialMask, member, "Doc", min, max) */
    PTX_END_FIELDS
    
    PTX_BEGIN_METHODS(MaterialMask)
        PTX_METHOD_AUTO(MaterialMask, SetOpacity, "Set opacity"),
        PTX_METHOD_AUTO(MaterialMask, GetOpacityReference, "Get opacity reference"),
        PTX_METHOD_AUTO(MaterialMask, SetShape, "Set shape"),
        PTX_METHOD_AUTO(MaterialMask, SetMaterialShape, "Set material shape"),
        PTX_METHOD_AUTO(MaterialMask, SetMaterialOuter, "Set material outer")
    PTX_END_METHODS
    
    PTX_BEGIN_DESCRIBE(MaterialMask)
        PTX_CTOR(MaterialMask, const IMaterial *, const IMaterial *, Shape *)
    PTX_END_DESCRIBE(MaterialMask)
    
};
