#pragma once
#include "../../material/imaterial.hpp"
#include "../../material/materialt.hpp"
#include "../../../../core/math/vector3d.hpp"
#include "../../../../core/color/rgbcolor.hpp"

// Parameter block  ------------------------------------------------------------
struct GradientShaderParams {
    uint8_t colorCount;
    RGBColor* rgbColors; ///< Array of colors used in the gradient.
    RGBColor* baseRGBColors; ///< Pointer to the base RGB colors.
    Vector2D positionOffset; ///< Offset for the gradient's position.
    Vector2D rotationOffset; ///< Point about which the gradient rotates.
    float gradientPeriod = 1.0f; ///< Period of the gradient repetition.
    float rotationAngle = 0.0f; ///< Angle for rotating the gradient.
    bool isRadial = false; ///< Whether the gradient is radial.
    bool isStepped = false; ///< Whether the gradient is stepped.
    float gradientShift = 0.0f; ///< Shift in the gradient pattern.
};

// Shader  ---------------------------------------------------------------------
class GradientShader final : public IShader {
public:
    /**
     * @brief Updates the colors of the gradient.
     *
     * @param rgbColors New array of colors for the gradient.
     */
    void UpdateGradient(RGBColor* rgbColors);

    /**
     * @brief Sets the position offset for the gradient.
     *
     * @param positionOffset The new position offset.
     */
    void SetPositionOffset(Vector2D positionOffset);

    /**
     * @brief Sets the rotation offset for the gradient.
     *
     * @param rotationOffset The new rotation offset point.
     */
    void SetRotationOffset(Vector2D rotationOffset);

    /**
     * @brief Sets the rotation angle for the gradient.
     *
     * @param rotationAngle The new rotation angle in degrees.
     */
    void SetRotationAngle(float rotationAngle);

    /**
     * @brief Sets the gradient's period of repetition.
     *
     * @param gradientPeriod The new gradient period.
     */
    void SetGradientPeriod(float gradientPeriod);

    /**
     * @brief Shifts the gradient by a ratio.
     *
     * @param ratio The ratio by which to shift the gradient.
     */
    void GradientShift(float ratio);

    /**
     * @brief Shifts the hue of the gradient by a specified degree.
     *
     * @param hueDeg Degrees by which to shift the gradient's hue.
     */
    void HueShift(float hueDeg);

    /**
     * @brief Updates the RGB colors in the gradient.
     */
    void UpdateRGB();

    RGBColor Shade(const SurfaceProperties& sp,
                   const IMaterial&         m) const override {
        // Compile-time-safe down-cast
        using NoiseMat = MaterialT<GradientShaderParams, 
                                   GradientShader>;

        const auto& mat = m.As<NoiseMat>();

        Vector3D positionL = sp.position;

        if (rotationAngle != 0) {
            Quaternion temp = Rotation(EulerAngles(Vector3D(0, 0, rotationAngle), EulerConstants::EulerOrderXYZS)).GetQuaternion();

            positionL = temp.RotateVector(positionL);
        }

        float pos = 0;
        positionL = positionL - Vector3D(positionOffset.X, positionOffset.Y, 0);
        positionL = positionL + Vector3D(gradientShift * gradientPeriod, 0, 0);

        if (isRadial) {
            pos = sqrtf(positionL.X * positionL.X + positionL.Y * positionL.Y);
            pos = fabs(fmodf(pos, gradientPeriod));
        }
        else {
            // from x position, fit into bucket ratio
            pos = fabs(fmodf(positionL.X, gradientPeriod));
        }

        // map from modulo'd x value to color count minimum
        float ratio = Mathematics::Map(pos, 0.0f, gradientPeriod, 0.0f, float(colorCount));
        uint8_t startBox = floor(ratio);
        uint8_t endBox = startBox + 1 >= (uint8_t)colorCount ? 0 : startBox + 1;

        RGBColor rgb;

        if (isStepped) {
            rgb = rgbColors[startBox];
        }
        else {
            float mu = Mathematics::Map(ratio, float(startBox), float(startBox + 1), 0.0f, 1.0f); // calculate mu between boxes

            rgb = RGBColor::InterpolateColors(rgbColors[startBox], rgbColors[endBox], mu);
        }

        return rgb;
    }
};
