#include "axisangle.hpp"

// Constructor with individual components.
AxisAngle::AxisAngle(float rotation, float x, float y, float z) : Rotation(rotation), Axis(x, y, z) {}

// Constructor with Vector3D.
AxisAngle::AxisAngle(float rotation, Vector3D axis) : Rotation(rotation), Axis(axis) {}

// Convert AxisAngle to a string representation.
uc3d::UString AxisAngle::ToString() {
    uc3d::UString r = Mathematics::DoubleToCleanString(Rotation);
    uc3d::UString x = Mathematics::DoubleToCleanString(Axis.X);
    uc3d::UString y = Mathematics::DoubleToCleanString(Axis.Y);
    uc3d::UString z = Mathematics::DoubleToCleanString(Axis.Z);

    return r + ": [" + x + " " + y + " " + z + "]";
}
