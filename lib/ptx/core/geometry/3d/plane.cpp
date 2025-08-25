#include "plane.hpp"

// Default constructor.
Plane::Plane() {}

// Parameterized constructor.
Plane::Plane(Vector3D centroid, Vector3D normal)
    : Centroid(centroid), Normal(normal) {}

// Convert Plane to a string representation.
ptx::UString Plane::ToString() {
    ptx::UString centroid = Centroid.ToString();
    ptx::UString normal = Normal.ToString();

    return "[ " + centroid + ", " + normal + " ]";
}
