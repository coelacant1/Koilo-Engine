#pragma once

#include <cstddef>  // for size_t
#include "../../../../registry/reflect_macros.hpp"

#include "../imaterial.hpp"
#include "../materialt.hpp"
#include "../../shader/ishader.hpp"

#include "../../shader/implementations/depthparams.hpp"
#include "../../shader/implementations/depthshader.hpp"

#include "gradientmaterial.hpp"
#include "../../../../core/color/rgbcolor.hpp"

/**
 * @file depthmaterial.hpp
 * @brief MaterialT<DepthParams,DepthShader> wrapper that owns a small 1D gradient.
 */

/**
 * @class DepthMaterial
 * @brief Depth-based material with an internal gradient source.
 *
 * Owns a @ref GradientMaterial of @ref kN colors and wires it to the parameter block so the
 * shader can sample the gradient during shading. Constructor sets axis, depth, offset and
 * binds the internal gradient.
 */
class DepthMaterial : public MaterialT<DepthParams, DepthShader> {
    using Base = MaterialT<DepthParams, DepthShader>;

public:
    /** @brief Number of colors in the default gradient spectrum. */
    static constexpr size_t kN = 4;

    /** @brief Internal gradient used by the shader via the params block. */
    GradientMaterial<kN> gradient_;

    /**
     * @brief Construct and bind the internal gradient; initialize axis/depth/offset.
     * @param axis    Depth projection axis.
     * @param depth   Depth scale factor.
     * @param zOffset Depth offset.
     */
    DepthMaterial(DepthAxis axis, float depth, float zOffset)
    : Base()
    , gradient_(spectrum_, /*period*/ 2.0f, /*isRadial*/ false)
    {
        this->axis    = axis;
        this->depth   = depth;
        this->offset  = zOffset;
        this->gradient = &gradient_;
    }

    /** @brief Set depth projection axis. */
    void SetAxis(DepthAxis a)     { this->axis = a; }

    /** @brief Set depth scale factor. */
    void SetDepth(float d)        { this->depth = d; }

    /** @brief Set depth offset. */
    void SetOffset(float off)     { this->offset = off; }

    /** @brief Access the underlying gradient for customization. */
    GradientMaterial<kN>& Gradient() { return gradient_; }

private:
    /** @brief Default spectrum read by @ref gradient_. */
    RGBColor spectrum_[kN] = {
        RGBColor(0, 255, 0),   // green
        RGBColor(255, 0, 0),   // red
        RGBColor(0, 255, 0),   // green
        RGBColor(0, 0, 255)    // blue
    };

    PTX_BEGIN_FIELDS(DepthMaterial)
        PTX_FIELD(DepthMaterial, gradient_, "Gradient", -2147483648, 2147483647)
    PTX_END_FIELDS
    
    PTX_BEGIN_METHODS(DepthMaterial)
        PTX_METHOD_AUTO(DepthMaterial, SetAxis, "Set axis"),
        PTX_METHOD_AUTO(DepthMaterial, SetDepth, "Set depth"),
        PTX_METHOD_AUTO(DepthMaterial, SetOffset, "Set offset"),
        PTX_METHOD_AUTO(DepthMaterial, Gradient, "Gradient")
    PTX_END_METHODS
    
    PTX_BEGIN_DESCRIBE(DepthMaterial)
        PTX_CTOR(DepthMaterial, DepthAxis, float, float)
    PTX_END_DESCRIBE(DepthMaterial)
    
};
