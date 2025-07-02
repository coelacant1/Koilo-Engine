#include "wait.hpp"


Wait::Wait(uint32_t millisToWait) {
    this->millisToWait = millisToWait;
}

void Wait::Reset() {
    previousMillis = um3d::Time::Millis();
}

bool Wait::IsFinished() {
    return um3d::Time::Millis() - previousMillis >= millisToWait;
}
