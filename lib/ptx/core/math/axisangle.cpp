#include "axisangle.hpp"

// Constructor with individual components.
AxisAngle::AxisAngle(float rotation, float x, float y, float z) : Rotation(rotation), Axis(x, y, z) {}

// Constructor with Vector3D.
AxisAngle::AxisAngle(float rotation, Vector3D axis) : Rotation(rotation), Axis(axis) {}

// Convert AxisAngle to a string representation.
ptx::UString AxisAngle::ToString() {
    ptx::UString r = Mathematics::DoubleToCleanString(Rotation);
    ptx::UString x = Mathematics::DoubleToCleanString(Axis.X);
    ptx::UString y = Mathematics::DoubleToCleanString(Axis.Y);
    ptx::UString z = Mathematics::DoubleToCleanString(Axis.Z);

    return r + ": [" + x + " " + y + " " + z + "]";
}
