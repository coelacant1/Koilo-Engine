// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/core/control/dampedspring.hpp>


namespace koilo {

koilo::DampedSpring::DampedSpring() :
    currentVelocity(0.001f),
    currentPosition(0.001f),
    springConstant(0.0f),
    springForce(0.0f),
    dampingForce(0.0f),
    force(0.0f),
    damping(0.0f),
    previousMillis(0) {
}

koilo::DampedSpring::DampedSpring(float springConstant, float damping) :
    currentVelocity(0.001f),
    currentPosition(0.001f),
    springConstant(-1.0f * springConstant),
    springForce(0.0f),
    dampingForce(0.0f),
    force(0.0f),
    damping(-1.0f * damping),
    previousMillis(0) {
}

float koilo::DampedSpring::GetCurrentPosition() {
    return currentPosition;
}

void koilo::DampedSpring::SetConstants(float springConstant, float damping) {
    this->springConstant = -1.0f * springConstant;
    this->damping = -1.0f * damping;
}

float koilo::DampedSpring::Calculate(float target, uint32_t currentMillis) {
    float dT = ((float)(currentMillis - previousMillis)) / 1000.0f;

    if (dT > 0.01f && dT < 2.0f) {
        springForce = springConstant * currentPosition;
        dampingForce = damping * currentVelocity;
        force = springForce - dampingForce + target;

        currentVelocity += force * dT;
        currentPosition += currentVelocity * dT;
    }

    previousMillis = currentMillis;

    return currentPosition;
}

float koilo::DampedSpring::Calculate(float target, float dT) {
    if (!Mathematics::IsClose(target, currentPosition, 0.01f)) {
        springForce = springConstant * currentPosition;
        dampingForce = damping * currentVelocity;
        force = springForce + dampingForce + target;

        currentVelocity += force * dT;
        currentPosition += currentVelocity * dT;
    }

    return currentPosition;
}

} // namespace koilo
