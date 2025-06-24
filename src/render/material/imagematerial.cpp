#include "../../../include/um3d/render/material/imagematerial.hpp"

ImageMaterial::ImageMaterial(Image* image){
    this->image = image;
}

void ImageMaterial::SetHueAngle(float hueAngle) {
    this->hueAngle = hueAngle;
}

RGBColor ImageMaterial::GetRGB(const Vector3D& position, const Vector3D& normal, const Vector3D& uvw) {
    return image->GetColorAtCoordinate(Vector2D(position.X, position.Y)).HueShift(hueAngle);
}
