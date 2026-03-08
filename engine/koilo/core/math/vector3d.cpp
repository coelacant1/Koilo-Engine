// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/core/math/vector3d.hpp>


namespace koilo {

koilo::UString koilo::Vector3D::ToString() const {
    koilo::UString x = Mathematics::DoubleToCleanString(this->X);
    koilo::UString y = Mathematics::DoubleToCleanString(this->Y);
    koilo::UString z = Mathematics::DoubleToCleanString(this->Z);

    return "[" + x + ", " + y + ", " + z + "]";
}

} // namespace koilo
