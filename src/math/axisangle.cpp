#include "include/um3d/math/axisangle.hpp"

// Constructor with individual components.
AxisAngle::AxisAngle(float rotation, float x, float y, float z) : Rotation(rotation), Axis(x, y, z) {}

// Constructor with Vector3D.
AxisAngle::AxisAngle(float rotation, Vector3D axis) : Rotation(rotation), Axis(axis) {}

// Convert AxisAngle to a string representation.
string_t AxisAngle::ToString() {
    string_t r = Mathematics::DoubleToCleanString(Rotation);
    string_t x = Mathematics::DoubleToCleanString(Axis.X);
    string_t y = Mathematics::DoubleToCleanString(Axis.Y);
    string_t z = Mathematics::DoubleToCleanString(Axis.Z);

    return r + ": [" + x + " " + y + " " + z + "]";
}
