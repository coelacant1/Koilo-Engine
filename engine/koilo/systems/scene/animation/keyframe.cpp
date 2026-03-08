// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/scene/animation/keyframe.hpp>


namespace koilo {

koilo::KeyFrame::KeyFrame() {}

koilo::KeyFrame::KeyFrame(float time, float value) {
    this->Time = time;
    this->Value = value;
}

void koilo::KeyFrame::Set(float time, float value) {
    this->Time = time;
    this->Value = value;
}

} // namespace koilo
