#include "../../../include/um3d/geometry/3d/plane.hpp"

// Default constructor.
Plane::Plane() {}

// Parameterized constructor.
Plane::Plane(Vector3D centroid, Vector3D normal)
    : Centroid(centroid), Normal(normal) {}

// Convert Plane to a string representation.
string_t Plane::ToString() {
    string_t centroid = Centroid.ToString();
    string_t normal = Normal.ToString();

    return "[ " + centroid + ", " + normal + " ]";
}
