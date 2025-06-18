#include "include/um3d/math/eulerangles.hpp"

// Default constructor.
EulerAngles::EulerAngles() 
    : Angles(0, 0, 0), Order(EulerConstants::EulerOrderXYZS) {}

// Constructor with angles and order.
EulerAngles::EulerAngles(Vector3D angles, EulerOrder order) 
    : Angles(angles), Order(order) {}

// Convert EulerAngles to a string representation.
string_t EulerAngles::ToString() {
    string_t angles = Angles.ToString();
    string_t order = Order.ToString();

    return "[ " + angles + ", " + order + " ]";
}
