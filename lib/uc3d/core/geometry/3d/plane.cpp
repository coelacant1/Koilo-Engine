#include "plane.hpp"

// Default constructor.
Plane::Plane() {}

// Parameterized constructor.
Plane::Plane(Vector3D centroid, Vector3D normal)
    : Centroid(centroid), Normal(normal) {}

// Convert Plane to a string representation.
uc3d::UString Plane::ToString() {
    uc3d::UString centroid = Centroid.ToString();
    uc3d::UString normal = Normal.ToString();

    return "[ " + centroid + ", " + normal + " ]";
}
