// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/core/time/timestep.hpp>


namespace koilo {

koilo::TimeStep::TimeStep(float frequency) {
    SetFrequency(frequency);
}

void koilo::TimeStep::SetFrequency(float frequency) {
    this->updateInterval = uint16_t((1.0f / frequency) * 1000.0f);
}

bool koilo::TimeStep::IsReady() {
    uint32_t currentMillis = koilo::Time::Millis();
    if (currentMillis - previousMillis >= updateInterval) {
        previousMillis = currentMillis;
        return true;
    } else {
        return false;
    }
}

} // namespace koilo
