// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/core/math/axisangle.hpp>


namespace koilo {

// Constructor with individual components.
koilo::AxisAngle::AxisAngle(float rotation, float x, float y, float z) : Rotation(rotation), Axis(x, y, z) {}

// Constructor with Vector3D.
koilo::AxisAngle::AxisAngle(float rotation, Vector3D axis) : Rotation(rotation), Axis(axis) {}

// Convert AxisAngle to a string representation.
koilo::UString koilo::AxisAngle::ToString() {
    koilo::UString r = Mathematics::DoubleToCleanString(Rotation);
    koilo::UString x = Mathematics::DoubleToCleanString(Axis.X);
    koilo::UString y = Mathematics::DoubleToCleanString(Axis.Y);
    koilo::UString z = Mathematics::DoubleToCleanString(Axis.Z);

    return r + ": [" + x + " " + y + " " + z + "]";
}

} // namespace koilo
