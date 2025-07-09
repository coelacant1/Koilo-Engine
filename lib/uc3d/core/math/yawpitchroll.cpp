#include "yawpitchroll.hpp"

YawPitchRoll::YawPitchRoll() : Yaw(0.0f), Pitch(0.0f), Roll(0.0f) {}

YawPitchRoll::YawPitchRoll(Vector3D vector) : Yaw(vector.X), Pitch(vector.Y), Roll(vector.Z) {}

YawPitchRoll::YawPitchRoll(const YawPitchRoll& ypr) : Yaw(ypr.Yaw), Pitch(ypr.Pitch), Roll(ypr.Roll) {}

YawPitchRoll::YawPitchRoll(float yaw, float pitch, float roll) : Yaw(yaw), Pitch(pitch), Roll(roll) {}

uc3d::UString YawPitchRoll::ToString() const {
    uc3d::UString y = Mathematics::DoubleToCleanString(this->Yaw);
    uc3d::UString p = Mathematics::DoubleToCleanString(this->Pitch);
    uc3d::UString r = Mathematics::DoubleToCleanString(this->Roll);

    return "[" + y + ", " + p + ", " + r + "]";
}
