#include <ptx/core/math/directionangle.hpp>

// Constructor with individual components.
DirectionAngle::DirectionAngle(float rotation, float x, float y, float z) 
    : Rotation(rotation), Direction(x, y, z) {}

// Constructor with Vector3D.
DirectionAngle::DirectionAngle(float rotation, Vector3D direction) 
    : Rotation(rotation), Direction(direction) {}

// Convert DirectionAngle to a string representation.
ptx::UString DirectionAngle::ToString() {
    ptx::UString r = Mathematics::DoubleToCleanString(Rotation);
    ptx::UString x = Mathematics::DoubleToCleanString(Direction.X);
    ptx::UString y = Mathematics::DoubleToCleanString(Direction.Y);
    ptx::UString z = Mathematics::DoubleToCleanString(Direction.Z);

    return r + ": [" + x + " " + y + " " + z + "]";
}
