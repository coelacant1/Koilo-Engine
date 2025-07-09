#include "directionangle.hpp"

// Constructor with individual components.
DirectionAngle::DirectionAngle(float rotation, float x, float y, float z) 
    : Rotation(rotation), Direction(x, y, z) {}

// Constructor with Vector3D.
DirectionAngle::DirectionAngle(float rotation, Vector3D direction) 
    : Rotation(rotation), Direction(direction) {}

// Convert DirectionAngle to a string representation.
uc3d::UString DirectionAngle::ToString() {
    uc3d::UString r = Mathematics::DoubleToCleanString(Rotation);
    uc3d::UString x = Mathematics::DoubleToCleanString(Direction.X);
    uc3d::UString y = Mathematics::DoubleToCleanString(Direction.Y);
    uc3d::UString z = Mathematics::DoubleToCleanString(Direction.Z);

    return r + ": [" + x + " " + y + " " + z + "]";
}
