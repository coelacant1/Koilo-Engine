#include "yawpitchroll.hpp"

YawPitchRoll::YawPitchRoll() : Yaw(0.0f), Pitch(0.0f), Roll(0.0f) {}

YawPitchRoll::YawPitchRoll(Vector3D vector) : Yaw(vector.X), Pitch(vector.Y), Roll(vector.Z) {}

YawPitchRoll::YawPitchRoll(const YawPitchRoll& ypr) : Yaw(ypr.Yaw), Pitch(ypr.Pitch), Roll(ypr.Roll) {}

YawPitchRoll::YawPitchRoll(float yaw, float pitch, float roll) : Yaw(yaw), Pitch(pitch), Roll(roll) {}

ptx::UString YawPitchRoll::ToString() const {
    ptx::UString y = Mathematics::DoubleToCleanString(this->Yaw);
    ptx::UString p = Mathematics::DoubleToCleanString(this->Pitch);
    ptx::UString r = Mathematics::DoubleToCleanString(this->Roll);

    return "[" + y + ", " + p + ", " + r + "]";
}
