// overlap.cpp
/**
 * @file overlap.cpp
 * @brief Implementations for Overlap2D shape-pair tests.
 * @date  25/06/2025
 */

#include <ptx/core/geometry/2d/overlap.hpp>
#include <ptx/core/math/mathematics.hpp>

/**
 * @brief Rectangle-Rectangle overlap with AABB fast path and SAT for rotated cases.
 */
bool Overlap2D::Overlaps(const Rectangle2D& a, const Rectangle2D& b){
    // Fast path: both rectangles ~axis-aligned -> AABB overlap
    if (Mathematics::IsClose(a.GetRotation(), 0.0f, 0.05f) &&
        Mathematics::IsClose(b.GetRotation(), 0.0f, 0.05f)){
        Shape::Bounds ba = a.GetBounds();
        Shape::Bounds bb = b.GetBounds();
        return !(ba.maxV.X < bb.minV.X || ba.minV.X > bb.maxV.X ||
                 ba.maxV.Y < bb.minV.Y || ba.minV.Y > bb.maxV.Y);
    } else {
        // SAT: gather corners (world space)
        const Rectangle2D::Corners ca = a.GetCorners();
        const Rectangle2D::Corners cb = b.GetCorners();

        // Edge normals for both rectangles (2 each)
        Vector2D axes[4] = {
            (ca.corners[1] - ca.corners[0]).Perpendicular(), // rect a
            (ca.corners[3] - ca.corners[0]).Perpendicular(), // rect a
            (cb.corners[1] - cb.corners[0]).Perpendicular(), // rect b
            (cb.corners[3] - cb.corners[0]).Perpendicular()  // rect b
        };

        for (unsigned char k = 0; k < 4; ++k) {
            Vector2D n = axes[k];

            // Project rectangle a
            float minA = ca.corners[0].DotProduct(n);
            float maxA = minA;
            for (unsigned char i = 1; i < 4; ++i) {
                float p = ca.corners[i].DotProduct(n);
                if (p < minA) minA = p;
                if (p > maxA) maxA = p;
            }

            // Project rectangle b
            float minB = cb.corners[0].DotProduct(n);
            float maxB = minB;
            for (unsigned char i = 1; i < 4; ++i) {
                float p = cb.corners[i].DotProduct(n);
                if (p < minB) minB = p;
                if (p > maxB) maxB = p;
            }

            // Separating gap?
            if (maxA < minB || maxB < minA) return false;
        }
        return true; // all axes overlapped
    }
}

/**
 * @brief Circle-Circle overlap via center distance vs. summed radii.
 */
bool Overlap2D::Overlaps(const Circle2D& a, const Circle2D& b){
    Vector2D ca = a.GetCenter();
    Vector2D cb = b.GetCenter();

    float dx = ca.X - cb.X;
    float dy = ca.Y - cb.Y;
    float r  = (a.GetSize().X + b.GetSize().X) / 4.0f; // (ra + rb); see notes below

    return (dx * dx + dy * dy) <= r * r;
}

/**
 * @brief Ellipse-Ellipse overlap (placeholder: uses bounding rectangles).
 * @note Replace with an actual ellipse-ellipse test if needed.
 */
bool Overlap2D::Overlaps(const Ellipse2D& a, const Ellipse2D& b){
    return Overlaps(Rectangle2D(a.GetBounds()), Rectangle2D(b.GetBounds()));
}

/**
 * @brief Triangle-Triangle overlap (placeholder: uses bounding rectangles).
 * @note Replace with SAT across triangle edges (and cross-axes) for precision.
 */
bool Overlap2D::Overlaps(const Triangle2D& a, const Triangle2D& b){
    return Overlaps(Rectangle2D(a.GetBounds()), Rectangle2D(b.GetBounds()));
}

/**
 * @brief Rectangle-Circle overlap using closest-point from circle center to rect AABB.
 */
bool Overlap2D::Overlaps(const Rectangle2D& r, const Circle2D& c){
    Shape::Bounds br = r.GetBounds();
    Vector2D cc = c.GetCenter();

    float cx = fmaxf(br.minV.X, fminf(cc.X, br.maxV.X));
    float cy = fmaxf(br.minV.Y, fminf(cc.Y, br.maxV.Y));
    float dx = cc.X - cx;
    float dy = cc.Y - cy;

    float rad = c.GetSize().X / 2.0f; // assumes X is diameter; adjust if size encodes radius
    return (dx * dx + dy * dy) <= (rad * rad);
}

/**
 * @brief Rectangle-Triangle overlap (placeholder: uses triangle bounds).
 */
bool Overlap2D::Overlaps(const Rectangle2D& r, const Triangle2D& t){
    return Overlaps(r, Rectangle2D(t.GetBounds()));
}

/**
 * @brief Rectangle-Ellipse overlap (placeholder: uses ellipse bounds).
 */
bool Overlap2D::Overlaps(const Rectangle2D& r, const Ellipse2D& e){
    return Overlaps(r, Rectangle2D(e.GetBounds()));
}

/**
 * @brief Circle-Triangle overlap (placeholder: uses triangle bounds).
 */
bool Overlap2D::Overlaps(const Circle2D& c, const Triangle2D& t){
    return Overlaps(Rectangle2D(t.GetBounds()), c);
}

/**
 * @brief Circle-Ellipse overlap (placeholder: uses ellipse bounds).
 */
bool Overlap2D::Overlaps(const Circle2D& c, const Ellipse2D& e){
    return Overlaps(Rectangle2D(e.GetBounds()), c);
}

/**
 * @brief Triangle-Ellipse overlap (placeholder: uses bounds of both shapes).
 */
bool Overlap2D::Overlaps(const Triangle2D& t, const Ellipse2D& e){
    return Overlaps(Rectangle2D(t.GetBounds()), Rectangle2D(e.GetBounds()));
}
