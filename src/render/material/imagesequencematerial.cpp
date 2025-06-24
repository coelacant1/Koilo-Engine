#include "../../../include/um3d/render/material/imagesequencematerial.hpp"

ImageSequenceMaterial::ImageSequenceMaterial(ImageSequence* imagesequence){
    this->imageSequence = imageSequence;
}

void ImageSequenceMaterial::Update(float ratio) {
    imageSequence->Update();
}

void ImageSequenceMaterial::SetHueAngle(float hueAngle) {
    this->hueAngle = hueAngle;
}

RGBColor ImageSequenceMaterial::GetRGB(const Vector3D& position, const Vector3D& normal, const Vector3D& uvw) {
    return imageSequence->GetColorAtCoordinate(Vector2D(position.X, position.Y)).HueShift(hueAngle);
}
