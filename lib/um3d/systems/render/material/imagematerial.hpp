/**
 * @file 
 * @brief 
 *
 *
 * @date 23/6/2025
 * @author Coela Can't
 */


#pragma once


#include "material.hpp" // Base class for materials.
#include "../../../assets/image/image.hpp"

class ImageMaterial : public Material {
private:
    Image* image;
    float hueAngle = 0.0f; ///< The hue adjustment angle of the image.

public:
    ImageMaterial(Image* image);

    /**
     * @brief Sets the hue adjustment angle of the image.
     *
     * @param hueAngle The new hue adjustment angle in degrees.
     */
    void SetHueAngle(float hueAngle);

    /**
     * @brief Calculates the RGB color at a specific position.
     *
     * @param position 3D position in the image.
     * @param normal Normal vector at the position.
     * @param uvw Texture coordinates at the position.
     * @return The calculated RGB color.
     */
    RGBColor GetRGB(const Vector3D& position, const Vector3D& normal, const Vector3D& uvw) override;
};
