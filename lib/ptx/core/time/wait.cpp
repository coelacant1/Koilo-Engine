#include "wait.hpp"


Wait::Wait(uint32_t millisToWait) {
    this->millisToWait = millisToWait;
}

void Wait::Reset() {
    previousMillis = ptx::Time::Millis();
}

bool Wait::IsFinished() {
    return ptx::Time::Millis() - previousMillis >= millisToWait;
}
