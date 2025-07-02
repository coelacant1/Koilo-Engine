#include "rastertriangle3d.hpp"

RasterTriangle3D::RasterTriangle3D()
    : p1(nullptr), p2(nullptr), p3(nullptr),
      uv1(nullptr), uv2(nullptr), uv3(nullptr),
      hasUV(false) {}


RasterTriangle3D::RasterTriangle3D(const Vector3D* v1, const Vector3D* v2, const Vector3D* v3)
    : p1(v1), p2(v2), p3(v3){
    
    if (p1 && p2 && p3) {
        edge1 = *p2 - *p1;
        edge2 = *p3 - *p1;
        normal = edge1.CrossProduct(edge2).UnitSphere();
    }

    hasUV = false;
}

RasterTriangle3D::RasterTriangle3D(const Vector3D* v1, const Vector3D* v2, const Vector3D* v3,
                                   const Vector2D* t1, const Vector2D* t2, const Vector2D* t3)
    : p1(v1), p2(v2), p3(v3),
      uv1(t1), uv2(t2), uv3(t3) {

    // Ensure vertex pointers are valid before dereferencing
    if (p1 && p2 && p3) {
        edge1 = *p2 - *p1;
        edge2 = *p3 - *p1;
        normal = edge1.CrossProduct(edge2).UnitSphere();
    }

    hasUV = true;
}

const Vector3D& RasterTriangle3D::GetNormal() const {
    return normal;
}

bool RasterTriangle3D::IntersectsRay(const Vector3D& rayOrigin, const Vector3D& rayDir,
                                   float& out_t, float& out_u, float& out_v) const {
    Vector3D pvec = rayDir.CrossProduct(edge2);
    float det = edge1.DotProduct(pvec);

    // If the determinant is near zero, the ray lies in the plane of the triangle or is parallel to it.
    if (Mathematics::FAbs(det) < Mathematics::EPSILON) {
        return false;
    }

    float invDet = 1.0f / det;
    Vector3D tvec = rayOrigin - *p1;

    // Calculate u parameter and test bounds
    out_u = tvec.DotProduct(pvec) * invDet;
    if (out_u < 0.0f || out_u > 1.0f) {
        return false;
    }

    // Calculate v parameter and test bounds
    Vector3D qvec = tvec.CrossProduct(edge1);
    out_v = rayDir.DotProduct(qvec) * invDet;
    if (out_v < 0.0f || out_u + out_v > 1.0f) {
        return false;
    }

    // Calculate t, the distance along the ray to the intersection point
    out_t = edge2.DotProduct(qvec) * invDet;

    // Ensure the intersection is in front of the ray's origin
    return (out_t > Mathematics::EPSILON);
}