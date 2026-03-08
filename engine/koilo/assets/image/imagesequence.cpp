// SPDX-License-Identifier: GPL-3.0-or-later
// imagesequence.cpp
#include <koilo/assets/image/imagesequence.hpp>
#include <cmath>


namespace koilo {

/**
 * @file imagesequence.cpp
 * @brief Implementation of ImageSequence playback and sampling.
 * @date 8/18/2025
 * @author Coela Can't
 */

koilo::ImageSequence::ImageSequence(Image* image, const uint8_t** data, unsigned int imageCount, float fps) {
    this->startTime = koilo::Time::Millis();
    this->image = image;
    this->data = data;
    this->imageCount = imageCount;
    this->fps = fps;
    this->frameTime = ((float)imageCount) / fps;
}

void koilo::ImageSequence::SetFPS(float fps) {
    this->fps = fps;
}

void koilo::ImageSequence::SetSize(Vector2D size) {
    image->SetSize(size);
}

void koilo::ImageSequence::SetPosition(Vector2D offset) {
    image->SetPosition(offset);
}

void koilo::ImageSequence::SetRotation(float angle) {
    image->SetRotation(angle);
}

void koilo::ImageSequence::Reset() {
    currentFrame = 0;
    startTime = koilo::Time::Millis();
}

void koilo::ImageSequence::Update() {
    // Normalize time [0,1) over full sequence duration, then map to [0, imageCount-1]
    float currentTime = fmod((koilo::Time::Millis() - startTime) / 1000.0f, frameTime) / frameTime;
    currentFrame = (unsigned int)Mathematics::Map(currentTime, 0.0f, 1.0f, 0.0f, float(imageCount - 1));
    image->SetData(data[currentFrame]);
}

koilo::Color888 koilo::ImageSequence::GetColorAtCoordinate(Vector2D point){
    return image->GetColorAtCoordinate(point);
}

} // namespace koilo
