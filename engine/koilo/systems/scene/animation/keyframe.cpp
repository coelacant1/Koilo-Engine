#include <ptx/systems/scene/animation/keyframe.hpp>

KeyFrame::KeyFrame() {}

KeyFrame::KeyFrame(float time, float value) {
    this->Time = time;
    this->Value = value;
}

void KeyFrame::Set(float time, float value) {
    this->Time = time;
    this->Value = value;
}
