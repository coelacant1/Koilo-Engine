#include "yawpitchroll.hpp"

YawPitchRoll::YawPitchRoll() : Yaw(0.0f), Pitch(0.0f), Roll(0.0f) {}

YawPitchRoll::YawPitchRoll(Vector3D vector) : Yaw(vector.X), Pitch(vector.Y), Roll(vector.Z) {}

YawPitchRoll::YawPitchRoll(const YawPitchRoll& ypr) : Yaw(ypr.Yaw), Pitch(ypr.Pitch), Roll(ypr.Roll) {}

YawPitchRoll::YawPitchRoll(float yaw, float pitch, float roll) : Yaw(yaw), Pitch(pitch), Roll(roll) {}

um3d::UString YawPitchRoll::ToString() const {
    um3d::UString y = Mathematics::DoubleToCleanString(this->Yaw);
    um3d::UString p = Mathematics::DoubleToCleanString(this->Pitch);
    um3d::UString r = Mathematics::DoubleToCleanString(this->Roll);

    return "[" + y + ", " + p + ", " + r + "]";
}
