#include <ptx/systems/render/raster/helpers/rastertriangle3d.hpp>

/**
 * @file rastertriangle3d.cpp
 * @brief 3D triangle used for rasterization and ray tests.
 *
 * Stores pointers to vertex/UV inputs, precomputes edges and a unit normal,
 * and exposes a ray-triangle intersection using the Möller-Trumbore test.
 */

/** @brief Default-constructs with null pointers and no UVs. */
RasterTriangle3D::RasterTriangle3D()
    : p1(nullptr), p2(nullptr), p3(nullptr),
      uv1(nullptr), uv2(nullptr), uv3(nullptr),
      hasUV(false) {}

/**
 * @brief Construct from three vertex pointers (no UVs).
 * @param v1 Pointer to first vertex.
 * @param v2 Pointer to second vertex.
 * @param v3 Pointer to third vertex.
 *
 * If all vertex pointers are valid, precomputes @ref edge1, @ref edge2 and a unit @ref normal.
 * Sets @ref hasUV to false.
 */
RasterTriangle3D::RasterTriangle3D(const Vector3D* v1, const Vector3D* v2, const Vector3D* v3)
    : p1(v1), p2(v2), p3(v3){
    
    if (p1 && p2 && p3) {
        edge1 = *p2 - *p1;
        edge2 = *p3 - *p1;
        normal = edge1.CrossProduct(edge2).UnitSphere();
    }

    hasUV = false;
}

/**
 * @brief Construct from three vertex pointers and three UV pointers.
 * @param v1 Pointer to first vertex.
 * @param v2 Pointer to second vertex.
 * @param v3 Pointer to third vertex.
 * @param t1 Pointer to first UV.
 * @param t2 Pointer to second UV.
 * @param t3 Pointer to third UV.
 *
 * If all vertex pointers are valid, precomputes @ref edge1, @ref edge2 and a unit @ref normal.
 * Sets @ref hasUV to true and forwards the UV pointers.
 */
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

/**
 * @brief Access the precomputed unit normal.
 * @return Reference to the triangle's unit-length normal.
 */
const Vector3D& RasterTriangle3D::GetNormal() const {
    return normal;
}

/**
 * @brief Ray-triangle intersection (Möller-Trumbore).
 * @param rayOrigin Ray origin.
 * @param rayDir    Ray direction.
 * @param out_t     On hit: distance along the ray to the intersection point.
 * @param out_u     On hit: barycentric u coordinate.
 * @param out_v     On hit: barycentric v coordinate.
 * @return true if the ray intersects the triangle at a positive distance; false otherwise.
 *
 * The test rejects rays parallel to the triangle plane (|det| < EPSILON) and
 * intersections outside the triangle (u < 0, v < 0, u + v > 1). A hit requires
 * out_t > EPSILON to ensure it lies in front of the origin.
 */
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
