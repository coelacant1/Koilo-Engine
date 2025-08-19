#include "rastertriangle2d.hpp"

/**
 * @file rastertriangle2d.cpp
 * @brief 2D rasterizable triangle built from a projected 3D source triangle.
 *
 * Projects a 3D triangle into screen space, caches a bounding box and barycentric
 * denominator for fast point-inside tests, and forwards material/UV pointers.
 */

/** @brief Default-constructs a degenerate 2D triangle with unit bounds. */
RasterTriangle2D::RasterTriangle2D()
    : Triangle2D(),
      t3p1(nullptr), t3p2(nullptr), t3p3(nullptr), normal(nullptr),
      material(nullptr), p1UV(nullptr), p2UV(nullptr), p3UV(nullptr),
      hasUV(false), averageDepth(0.0f), denominator(0.0f), bounds(Rectangle2D(Vector2D(0.0f, 0.0f), Vector2D(1.0f, 1.0f))) {}

/**
 * @brief Build a raster triangle by projecting a 3D source triangle through a camera.
 * @param camTransform  Camera transform (position/rotation/scale).
 * @param lookDirection Camera look quaternion (multiplied with cam rotation).
 * @param sourceTriangle Source 3D triangle (positions/normal/UVs).
 * @param mat            Material pointer associated with the triangle (non-owning).
 */
RasterTriangle2D::RasterTriangle2D(const Transform& camTransform, const Quaternion& lookDirection,
                                   const RasterTriangle3D& sourceTriangle, IMaterial* mat)
    : bounds(Rectangle2D(Vector2D(0.0f, 0.0f), Vector2D(1.0f, 1.0f)))
{
    // --- Assign pointers to original 3D data ---
    this->material = mat;
    this->t3p1 = sourceTriangle.p1;
    this->t3p2 = sourceTriangle.p2;
    this->t3p3 = sourceTriangle.p3;
    this->normal = &sourceTriangle.normal;

    // --- Copy UV data if available ---
    this->hasUV = sourceTriangle.hasUV;
    if (this->hasUV) {
        this->p1UV = sourceTriangle.uv1;
        this->p2UV = sourceTriangle.uv2;
        this->p3UV = sourceTriangle.uv3;
    }

    // --- Project 3D vertices to 2D screen space ---
    Quaternion inverseCamRotation = camTransform.GetRotation().Multiply(lookDirection).Conjugate();
    Vector3D projectedP1 = inverseCamRotation.RotateVector(*t3p1 - camTransform.GetPosition()) / camTransform.GetScale();
    Vector3D projectedP2 = inverseCamRotation.RotateVector(*t3p2 - camTransform.GetPosition()) / camTransform.GetScale();
    Vector3D projectedP3 = inverseCamRotation.RotateVector(*t3p3 - camTransform.GetPosition()) / camTransform.GetScale();

    // --- Set the 2D vertices in the base class ---
    this->p1 = Vector2D(projectedP1.X, projectedP1.Y);
    this->p2 = Vector2D(projectedP2.X, projectedP2.Y);
    this->p3 = Vector2D(projectedP3.X, projectedP3.Y);

    // --- Calculate rendering data ---
    this->averageDepth = (projectedP1.Z + projectedP2.Z + projectedP3.Z) / 3.0f;

    // --- Pre-calculate bounds and denominator for efficiency ---
    CalculateBoundsAndDenominator();
}

/**
 * @brief Precompute AABB and cached values for barycentric tests.
 *
 * Computes edge vectors (v0, v1), the barycentric denominator (stored as 1/det
 * for a single multiply in queries), and the axis-aligned bounding box.
 * Degenerate triangles set @ref denominator to 0.0f.
 */
void RasterTriangle2D::CalculateBoundsAndDenominator() {
    // --- Calculate edge vectors for barycentric coordinates ---
    v0 = p2 - p1;
    v1 = p3 - p1;

    // --- Pre-calculate denominator for barycentric calculations ---
    denominator = (v0.X * v1.Y - v1.X * v0.Y);
    // Avoid division by zero for degenerate triangles
    if (Mathematics::FAbs(denominator) > Mathematics::EPSILON) {
        denominator = 1.0f / denominator;
    } else {
        denominator = 0.0f; // Mark as degenerate
    }

    // --- Calculate the Axis-Aligned Bounding Box (AABB) ---
    float minX = Mathematics::Min(p1.X, p2.X, p3.X);
    float minY = Mathematics::Min(p1.Y, p2.Y, p3.Y);
    float maxX = Mathematics::Max(p1.X, p2.X, p3.X);
    float maxY = Mathematics::Max(p1.Y, p2.Y, p3.Y);
    this->bounds = Rectangle2D(Vector2D(minX, minY), Vector2D(maxX, maxY));
}

/**
 * @brief Compute barycentric coordinates for a point and test containment.
 * @param x Input point X.
 * @param y Input point Y.
 * @param u Output barycentric u.
 * @param v Output barycentric v.
 * @param w Output barycentric w.
 * @return true if the point lies inside the triangle (including edges).
 */
bool RasterTriangle2D::GetBarycentricCoords(float x, float y, float& u, float& v, float& w) const {
    // If triangle is degenerate, no point is inside.
    if (Mathematics::FAbs(denominator) < Mathematics::EPSILON) return false;

    // Vector from the point to the triangle's first vertex
    Vector2D v2 = Vector2D(x, y) - p1;

    // Calculate barycentric coordinates v and w
    v = (v2.X * v1.Y - v1.X * v2.Y) * denominator;
    w = (v0.X * v2.Y - v2.X * v0.Y) * denominator;
    u = 1.0f - v - w;

    // The point is inside if all coordinates are non-negative
    return (v >= 0.0f) && (w >= 0.0f) && (u >= 0.0f);
}

/**
 * @brief AABB overlap query against the precomputed triangle bounds.
 * @param otherBounds Rectangle to test.
 * @return true if @ref bounds overlaps @p otherBounds.
 */
bool RasterTriangle2D::Overlaps(const Rectangle2D& otherBounds) const {
    // Simple AABB intersection test, suitable for a QuadTree.
    return this->bounds.Overlaps(otherBounds);
}

/** @brief Get the material associated with this triangle (non-owning). */
IMaterial* RasterTriangle2D::GetMaterial() const {
    return material;
}

/** @brief String dump of vertex positions: "p1 p2 p3". */
uc3d::UString RasterTriangle2D::ToString() const {
    return p1.ToString() + " " + p2.ToString() + " " + p3.ToString();
}
