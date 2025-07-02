#include "axisangle.hpp"

// Constructor with individual components.
AxisAngle::AxisAngle(float rotation, float x, float y, float z) : Rotation(rotation), Axis(x, y, z) {}

// Constructor with Vector3D.
AxisAngle::AxisAngle(float rotation, Vector3D axis) : Rotation(rotation), Axis(axis) {}

// Convert AxisAngle to a string representation.
um3d::UString AxisAngle::ToString() {
    um3d::UString r = Mathematics::DoubleToCleanString(Rotation);
    um3d::UString x = Mathematics::DoubleToCleanString(Axis.X);
    um3d::UString y = Mathematics::DoubleToCleanString(Axis.Y);
    um3d::UString z = Mathematics::DoubleToCleanString(Axis.Z);

    return r + ": [" + x + " " + y + " " + z + "]";
}
