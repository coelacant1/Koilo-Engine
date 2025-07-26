#pragma once

#include "gradientparams.hpp"
#include "../../material/imaterial.hpp"
#include "../../material/materialt.hpp"
#include "../../../../core/math/vector3d.hpp"
#include "../../../../core/color/rgbcolor.hpp"
#include "../../../../core/math/rotation.hpp"
#include "../../../../core/math/eulerconstants.hpp"

class GradientShader final : public IShader {
public:
    RGBColor Shade(const SurfaceProperties& sp,
                   const IMaterial&         m) override {
        // Compile-time-safe down-cast
        using NoiseMat = MaterialT<GradientParams, 
                                   GradientShader>;

        const auto& mat = m.As<NoiseMat>();

        Vector3D positionL = sp.position;

        if (mat.rotationAngle != 0) {
            Quaternion temp = Rotation(EulerAngles(Vector3D(0, 0, mat.rotationAngle), EulerConstants::EulerOrderXYZS)).GetQuaternion();

            positionL = temp.RotateVector(positionL);
        }

        float pos = 0;
        positionL = positionL - Vector3D(mat.positionOffset.X, mat.positionOffset.Y, 0);
        positionL = positionL + Vector3D(mat.gradientShift * mat.gradientPeriod, 0, 0);

        if (mat.isRadial) {
            pos = sqrtf(positionL.X * positionL.X + positionL.Y * positionL.Y);
            pos = fabs(fmodf(pos, mat.gradientPeriod));
        }
        else {
            // from x position, fit into bucket ratio
            pos = fabs(fmodf(positionL.X, mat.gradientPeriod));
        }

        // map from modulo'd x value to color count minimum
        float ratio = Mathematics::Map(pos, 0.0f, mat.gradientPeriod, 0.0f, float(mat.colorCount));
        uint8_t startBox = floor(ratio);
        uint8_t endBox = startBox + 1 >= (uint8_t)mat.colorCount ? 0 : startBox + 1;

        RGBColor rgb;

        if (mat.isStepped) {
            rgb = mat.rgbColors[startBox];
        }
        else {
            float mu = Mathematics::Map(ratio, float(startBox), float(startBox + 1), 0.0f, 1.0f); // calculate mu between boxes

            rgb = RGBColor::InterpolateColors(mat.rgbColors[startBox], mat.rgbColors[endBox], mu);
        }

        return rgb;
    }
};
