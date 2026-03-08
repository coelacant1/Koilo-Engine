// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/core/time/wait.hpp>



namespace koilo {

koilo::Wait::Wait(uint32_t millisToWait) {
    this->millisToWait = millisToWait;
    Reset();
}

void koilo::Wait::Reset() {
    previousMillis = koilo::Time::Millis();
}

bool koilo::Wait::IsFinished() {
    return koilo::Time::Millis() - previousMillis >= millisToWait;
}

} // namespace koilo
