#include "include/um3d/time/wait.hpp"


Wait::Wait(uint32_t millisToWait) {
    this->millisToWait = millisToWait;
}

void Wait::Reset() {
    previousMillis = millis();
}

bool Wait::IsFinished() {
    return millis() - previousMillis >= millisToWait;
}
