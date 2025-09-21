#pragma once

#include "../ishader.hpp"
#include "../../material/materialt.hpp"
#include "materialmaskparams.hpp"
#include "../../../../registry/reflect_macros.hpp"

#include "../../../../core/math/mathematics.hpp"
#include "../../../../core/math/vector2d.hpp"
#include "../../../../core/color/rgbcolor.hpp"

/**
 * @file materialmaskshader.hpp
 * @brief Shader that blends two child materials using a geometric mask shape.
 *
 * Evaluates a mask shape against the surface position to choose between an
 * “outer” material and a “shape” (inner) material. When inside the shape,
 * blends inner over outer by an opacity factor; otherwise uses the outer.
 */

class MaterialMaskShader final : public IShader {
public:
    /**
     * @brief Shade a surface point by masking between two materials.
     * @param surf Surface properties (position, etc.).
     * @param m    Material expected to be a MaterialT<MaterialMaskParams, MaterialMaskShader>.
     * @return Computed color after applying mask and optional blend.
     *
     * Behavior:
     * - If neither child material is present, returns black.
     * - If no shape is provided, shades using whichever child exists (outer preferred).
     * - If inside the shape: returns inner when opacity≈1; otherwise interpolates inner over outer.
     * - If outside the shape: returns outer if present; otherwise black.
     */
    RGBColor Shade(const SurfaceProperties& surf, const IMaterial& m) const override {
        using MaskMatBase = MaterialT<MaterialMaskParams, MaterialMaskShader>;
        const auto& p = m.As<MaskMatBase>();

        // Fallbacks if shape or children are missing
        if (!p.materialOuter && !p.materialShape) return {0,0,0};
        if (!p.shape) {
            // no shape => just use outer (or shape if outer missing)
            const IMaterial* child = p.materialOuter ? p.materialOuter : p.materialShape;
            return child->GetShader()->Shade(surf, *child);
        }

        const bool inShape = p.shape->IsInShape(Vector2D(surf.position.X, surf.position.Y));
        const float a = Mathematics::Constrain(p.opacity, 0.0f, 1.0f);

        if (inShape) {
            if (!p.materialShape) { // nothing to show inside
                if (!p.materialOuter) return {0,0,0};
                return p.materialOuter->GetShader()->Shade(surf, p.materialOuter ? *p.materialOuter : m);
            }

            if (a >= 0.97f) {
                return p.materialShape->GetShader()->Shade(surf, *p.materialShape);
            }

            // Blend shape over outer
            RGBColor shapeC = p.materialShape->GetShader()->Shade(surf, *p.materialShape);
            RGBColor outerC = p.materialOuter
                              ? p.materialOuter->GetShader()->Shade(surf, *p.materialOuter)
                              : RGBColor{0,0,0};

            // InterpolateColors expects factor 0..1
            return RGBColor::InterpolateColors(outerC, shapeC, a);
        } else {
            // outside the shape -> outer material
            if (!p.materialOuter) return {0,0,0};
            return p.materialOuter->GetShader()->Shade(surf, *p.materialOuter);
        }
    }

    PTX_BEGIN_FIELDS(MaterialMaskShader)
        /* No reflected fields. */
    PTX_END_FIELDS

    PTX_BEGIN_METHODS(MaterialMaskShader)
        PTX_METHOD_AUTO(MaterialMaskShader, Shade, "Shade")
    PTX_END_METHODS

    PTX_BEGIN_DESCRIBE(MaterialMaskShader)
        /* No reflected ctors. */
    PTX_END_DESCRIBE(MaterialMaskShader)

};
