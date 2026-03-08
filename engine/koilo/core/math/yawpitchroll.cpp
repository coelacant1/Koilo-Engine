// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/core/math/yawpitchroll.hpp>


namespace koilo {

koilo::YawPitchRoll::YawPitchRoll() : Yaw(0.0f), Pitch(0.0f), Roll(0.0f) {}

koilo::YawPitchRoll::YawPitchRoll(Vector3D vector) : Yaw(vector.X), Pitch(vector.Y), Roll(vector.Z) {}

koilo::YawPitchRoll::YawPitchRoll(const YawPitchRoll& ypr) : Yaw(ypr.Yaw), Pitch(ypr.Pitch), Roll(ypr.Roll) {}

koilo::YawPitchRoll::YawPitchRoll(float yaw, float pitch, float roll) : Yaw(yaw), Pitch(pitch), Roll(roll) {}

koilo::UString koilo::YawPitchRoll::ToString() const {
    koilo::UString y = Mathematics::DoubleToCleanString(this->Yaw);
    koilo::UString p = Mathematics::DoubleToCleanString(this->Pitch);
    koilo::UString r = Mathematics::DoubleToCleanString(this->Roll);

    return "[" + y + ", " + p + ", " + r + "]";
}

} // namespace koilo
