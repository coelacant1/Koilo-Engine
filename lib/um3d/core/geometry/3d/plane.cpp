#include "plane.hpp"

// Default constructor.
Plane::Plane() {}

// Parameterized constructor.
Plane::Plane(Vector3D centroid, Vector3D normal)
    : Centroid(centroid), Normal(normal) {}

// Convert Plane to a string representation.
um3d::UString Plane::ToString() {
    um3d::UString centroid = Centroid.ToString();
    um3d::UString normal = Normal.ToString();

    return "[ " + centroid + ", " + normal + " ]";
}
