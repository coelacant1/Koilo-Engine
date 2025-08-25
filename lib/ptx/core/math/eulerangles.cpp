#include "eulerangles.hpp"

// Default constructor.
EulerAngles::EulerAngles() 
    : Angles(0, 0, 0), Order(EulerConstants::EulerOrderXYZS) {}

// Constructor with angles and order.
EulerAngles::EulerAngles(Vector3D angles, EulerOrder order) 
    : Angles(angles), Order(order) {}

// Convert EulerAngles to a string representation.
ptx::UString EulerAngles::ToString() {
    ptx::UString angles = Angles.ToString();
    ptx::UString order = Order.ToString();

    return "[ " + angles + ", " + order + " ]";
}
