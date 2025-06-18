#include "include/um3d/math/yawpitchroll.hpp"

YawPitchRoll::YawPitchRoll() : Yaw(0.0f), Pitch(0.0f), Roll(0.0f) {}

YawPitchRoll::YawPitchRoll(Vector3D vector) : Yaw(vector.X), Pitch(vector.Y), Roll(vector.Z) {}

YawPitchRoll::YawPitchRoll(const YawPitchRoll& ypr) : Yaw(ypr.Yaw), Pitch(ypr.Pitch), Roll(ypr.Roll) {}

YawPitchRoll::YawPitchRoll(float yaw, float pitch, float roll) : Yaw(yaw), Pitch(pitch), Roll(roll) {}

string_t YawPitchRoll::ToString() const {
    string_t y = Mathematics::DoubleToCleanString(this->Yaw);
    string_t p = Mathematics::DoubleToCleanString(this->Pitch);
    string_t r = Mathematics::DoubleToCleanString(this->Roll);

    return "[" + y + ", " + p + ", " + r + "]";
}
