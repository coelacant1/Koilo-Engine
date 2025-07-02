#include "directionangle.hpp"

// Constructor with individual components.
DirectionAngle::DirectionAngle(float rotation, float x, float y, float z) 
    : Rotation(rotation), Direction(x, y, z) {}

// Constructor with Vector3D.
DirectionAngle::DirectionAngle(float rotation, Vector3D direction) 
    : Rotation(rotation), Direction(direction) {}

// Convert DirectionAngle to a string representation.
um3d::UString DirectionAngle::ToString() {
    um3d::UString r = Mathematics::DoubleToCleanString(Rotation);
    um3d::UString x = Mathematics::DoubleToCleanString(Direction.X);
    um3d::UString y = Mathematics::DoubleToCleanString(Direction.Y);
    um3d::UString z = Mathematics::DoubleToCleanString(Direction.Z);

    return r + ": [" + x + " " + y + " " + z + "]";
}
