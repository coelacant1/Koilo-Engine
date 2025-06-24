#pragma once

#include "../../render/material/animatedmaterial.hpp" // Include for base Material functionality.
#include "../../assets/image/imagesequence.hpp"

class ImageSequenceMaterial : public AnimatedMaterial {
private:
    ImageSequence* imageSequence;
    float hueAngle = 0.0f; ///< The hue adjustment angle of the image.

public:
    ImageSequenceMaterial(ImageSequence* imagesequence);

    void Update(float ratio) override;

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
