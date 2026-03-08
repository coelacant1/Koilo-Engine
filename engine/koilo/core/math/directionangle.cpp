// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/core/math/directionangle.hpp>


namespace koilo {

// Constructor with individual components.
koilo::DirectionAngle::DirectionAngle(float rotation, float x, float y, float z) 
    : Rotation(rotation), Direction(x, y, z) {}

// Constructor with Vector3D.
koilo::DirectionAngle::DirectionAngle(float rotation, Vector3D direction) 
    : Rotation(rotation), Direction(direction) {}

// Convert DirectionAngle to a string representation.
koilo::UString koilo::DirectionAngle::ToString() {
    koilo::UString r = Mathematics::DoubleToCleanString(Rotation);
    koilo::UString x = Mathematics::DoubleToCleanString(Direction.X);
    koilo::UString y = Mathematics::DoubleToCleanString(Direction.Y);
    koilo::UString z = Mathematics::DoubleToCleanString(Direction.Z);

    return r + ": [" + x + " " + y + " " + z + "]";
}

} // namespace koilo
