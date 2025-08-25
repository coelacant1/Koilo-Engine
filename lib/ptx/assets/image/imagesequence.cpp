// imagesequence.cpp
#include "imagesequence.hpp"

/**
 * @file imagesequence.cpp
 * @brief Implementation of ImageSequence playback and sampling.
 * @date 8/18/2025
 * @author Coela Can't
 */

ImageSequence::ImageSequence(Image* image, const uint8_t** data, unsigned int imageCount, float fps) {
    this->startTime = ptx::Time::Millis();
    this->image = image;
    this->data = data;
    this->imageCount = imageCount;
    this->fps = fps;
    this->frameTime = ((float)imageCount) / fps;
}

void ImageSequence::SetFPS(float fps) {
    this->fps = fps;
}

void ImageSequence::SetSize(Vector2D size) {
    image->SetSize(size);
}

void ImageSequence::SetPosition(Vector2D offset) {
    image->SetPosition(offset);
}

void ImageSequence::SetRotation(float angle) {
    image->SetRotation(angle);
}

void ImageSequence::Reset() {
    currentFrame = 0;
    startTime = ptx::Time::Millis();
}

void ImageSequence::Update() {
    // Normalize time [0,1) over full sequence duration, then map to [0, imageCount-1]
    float currentTime = fmod((ptx::Time::Millis() - startTime) / 1000.0f, frameTime) / frameTime;
    currentFrame = (unsigned int)Mathematics::Map(currentTime, 0.0f, 1.0f, 0.0f, float(imageCount - 1));
    image->SetData(data[currentFrame]);
}

RGBColor ImageSequence::GetColorAtCoordinate(Vector2D point){
    return image->GetColorAtCoordinate(point);
}
