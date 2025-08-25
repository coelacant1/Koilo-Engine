#include "pid.hpp"

PID::PID() :
    integral(0.0f),
    error(0.0f),
    previousError(0.0f),
    output(0.0f),
    kp(1.0f),
    ki(0.0f),
    kd(0.0f),
    previousSeconds(0.0f) {}

PID::PID(float kp, float ki, float kd) :
    integral(0.0f),
    error(0.0f),
    previousError(0.0f),
    output(0.0f),
    kp(kp),
    ki(ki),
    kd(kd),
    previousSeconds(0.0f) {}

PID::~PID() {}

float PID::Calculate(float setpoint, float processVariable, uint32_t currentMillis) {
    float POut, IOut, DOut;

    float currentSeconds = currentMillis / 1000.0f;
    float dT = currentSeconds - previousSeconds;

    error = setpoint - processVariable;
    integral += error * dT;

    POut = kp * error;
    IOut = ki * integral;
    DOut = kd * ((error - previousError) / dT);

    output = POut + IOut + DOut;
    previousError = error;

    previousSeconds = currentSeconds;

    return output;
}

float PID::Calculate(float setpoint, float processVariable, float dT) {
    float POut, IOut, DOut;

    error = setpoint - processVariable;
    integral += error * dT;

    POut = kp * error;
    IOut = ki * integral;
    DOut = kd * ((error - previousError) / dT);

    output = POut + IOut + DOut;
    previousError = error;

    return output;
}
