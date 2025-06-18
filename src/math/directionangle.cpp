#include "include/um3d/math/directionangle.hpp"

// Constructor with individual components.
DirectionAngle::DirectionAngle(float rotation, float x, float y, float z) 
    : Rotation(rotation), Direction(x, y, z) {}

// Constructor with Vector3D.
DirectionAngle::DirectionAngle(float rotation, Vector3D direction) 
    : Rotation(rotation), Direction(direction) {}

// Convert DirectionAngle to a string representation.
string_t DirectionAngle::ToString() {
    string_t r = Mathematics::DoubleToCleanString(Rotation);
    string_t x = Mathematics::DoubleToCleanString(Direction.X);
    string_t y = Mathematics::DoubleToCleanString(Direction.Y);
    string_t z = Mathematics::DoubleToCleanString(Direction.Z);

    return r + ": [" + x + " " + y + " " + z + "]";
}
