// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/core/math/eulerangles.hpp>


namespace koilo {

// Default constructor.
koilo::EulerAngles::EulerAngles() 
    : Angles(0, 0, 0), Order(EulerConstants::EulerOrderXYZS) {}

// Constructor with angles and order.
koilo::EulerAngles::EulerAngles(Vector3D angles, EulerOrder order) 
    : Angles(angles), Order(order) {}

// Convert EulerAngles to a string representation.
koilo::UString koilo::EulerAngles::ToString() {
    koilo::UString angles = Angles.ToString();
    koilo::UString order = Order.ToString();

    return "[ " + angles + ", " + order + " ]";
}

} // namespace koilo
