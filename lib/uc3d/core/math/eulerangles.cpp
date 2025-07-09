#include "eulerangles.hpp"

// Default constructor.
EulerAngles::EulerAngles() 
    : Angles(0, 0, 0), Order(EulerConstants::EulerOrderXYZS) {}

// Constructor with angles and order.
EulerAngles::EulerAngles(Vector3D angles, EulerOrder order) 
    : Angles(angles), Order(order) {}

// Convert EulerAngles to a string representation.
uc3d::UString EulerAngles::ToString() {
    uc3d::UString angles = Angles.ToString();
    uc3d::UString order = Order.ToString();

    return "[ " + angles + ", " + order + " ]";
}
