#include "../../../include/um3d/geometry/3d/triangle.hpp"

Triangle3D::Triangle3D()
    : p1(), p2(), p3(),
      uv1(nullptr), uv2(nullptr), uv3(nullptr) {}

Triangle3D::Triangle3D(const Vector3D& p1In,
                       const Vector3D& p2In,
                       const Vector3D& p3In,
                       const Vector2D* uv1In,
                       const Vector2D* uv2In,
                       const Vector2D* uv3In)
    : p1(p1In), p2(p2In), p3(p3In),
      uv1(uv1In), uv2(uv2In), uv3(uv3In) {}

Vector3D Triangle3D::GetNormal() const
{
    Vector3D edge1 = p2 - p1;
    Vector3D edge2 = p3 - p1;
    return edge1.CrossProduct(edge2).UnitSphere();
}

bool Triangle3D::IntersectRay(const Vector3D& rayOrigin,
                              const Vector3D& rayDir,
                              float*          tOut,
                              float*          uvOut) const
{
    const float eps = 1e-6f;
    Vector3D edge1 = p2 - p1;
    Vector3D edge2 = p3 - p1;
    Vector3D h     = rayDir.CrossProduct(edge2);
    float det      = edge1.DotProduct(h);
    if (det > -eps && det < eps) { return false; }

    float invDet = 1.0f / det;
    Vector3D s   = rayOrigin - p1;
    float u      = invDet * s.DotProduct(h);
    if (u < 0.0f || u > 1.0f) { return false; }

    Vector3D q = s.CrossProduct(edge1);
    float v    = invDet * rayDir.DotProduct(q);
    if (v < 0.0f || u + v > 1.0f) { return false; }

    float t = invDet * edge2.DotProduct(q);
    if (t < eps) { return false; }

    if (tOut)  { *tOut = t; }
    if (uvOut) { uvOut[0] = u; uvOut[1] = v; }
    return true;
}
