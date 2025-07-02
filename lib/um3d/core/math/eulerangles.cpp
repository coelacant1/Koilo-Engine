#include "eulerangles.hpp"

// Default constructor.
EulerAngles::EulerAngles() 
    : Angles(0, 0, 0), Order(EulerConstants::EulerOrderXYZS) {}

// Constructor with angles and order.
EulerAngles::EulerAngles(Vector3D angles, EulerOrder order) 
    : Angles(angles), Order(order) {}

// Convert EulerAngles to a string representation.
um3d::UString EulerAngles::ToString() {
    um3d::UString angles = Angles.ToString();
    um3d::UString order = Order.ToString();

    return "[ " + angles + ", " + order + " ]";
}
