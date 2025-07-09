#include "wait.hpp"


Wait::Wait(uint32_t millisToWait) {
    this->millisToWait = millisToWait;
}

void Wait::Reset() {
    previousMillis = uc3d::Time::Millis();
}

bool Wait::IsFinished() {
    return uc3d::Time::Millis() - previousMillis >= millisToWait;
}
