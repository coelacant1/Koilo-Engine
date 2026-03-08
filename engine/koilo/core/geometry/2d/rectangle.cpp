// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/core/geometry/2d/rectangle.hpp>
#include <koilo/core/geometry/2d/circle.hpp>
#include <cmath>


namespace koilo {

koilo::Rectangle2D::Rectangle2D(Vector2D center, Vector2D size, float rotation) : Shape(center, size, rotation) {}

koilo::Rectangle2D::Rectangle2D(Bounds bounds, float rotationDeg) : Shape(bounds, rotationDeg) {
    // Initialize Rectangle2D-specific member variables from bounds
    minV = bounds.minV;
    maxV = bounds.maxV;
    midV = Vector2D((minV.X + maxV.X) * 0.5f, (minV.Y + maxV.Y) * 0.5f);
}

bool koilo::Rectangle2D::IsInShape(Vector2D p) {
    Vector2D center = GetCenter();
    Vector2D size = GetSize();

    const float dx = p.X - center.X;
    const float dy = p.Y - center.Y;

    const float sinR = std::sin(rotation * Mathematics::MPID180);
    const float cosR = std::cos(rotation * Mathematics::MPID180);

    const float xLocal =  dx * cosR - dy * sinR;
    const float yLocal =  dx * sinR + dy * cosR;

    return Mathematics::FAbs(xLocal) <= size.X * 0.5f && Mathematics::FAbs(yLocal) <= size.Y * 0.5f;
}

koilo::Rectangle2D::Corners koilo::Rectangle2D::GetCorners() const {
    Corners c;

    // half-size in world units
    Vector2D size  = GetSize();
    Vector2D half  = { size.X * 0.5f, size.Y * 0.5f };
    Vector2D cen   = GetCenter();

    // axis-aligned corners before rotation
    c.corners[0] = { cen.X - half.X, cen.Y - half.Y };   /* ll */
    c.corners[1] = { cen.X + half.X, cen.Y - half.Y };   /* lr */
    c.corners[2] = { cen.X + half.X, cen.Y + half.Y };   /* ur */
    c.corners[3] = { cen.X - half.X, cen.Y + half.Y };   /* ul */

    // if rectangle has rotation, rotate each corner in place
    if (!Mathematics::IsClose(rotation, 0.0f, 0.001f)) {
        for (unsigned char i = 0; i < 4; ++i) {
            c.corners[i] = c.corners[i].Rotate(rotation, cen);
        }
    }
    return c;
}

void koilo::Rectangle2D::UpdateBounds(const Vector2D& v){
    minV = minV.Minimum(v);
    maxV = maxV.Maximum(v);
    midV = (minV + maxV) * 0.5f;
}

Vector2D koilo::Rectangle2D::GetMinimum() const {
    return minV; 
}

Vector2D koilo::Rectangle2D::GetMaximum() const {
    return maxV;
}

Vector2D koilo::Rectangle2D::GetCenter()  const {
    return midV;
}

bool koilo::Rectangle2D::Overlaps(const Rectangle2D& other) const{
    return Overlaps(other.minV, other.maxV);
}

bool koilo::Rectangle2D::Overlaps(const Vector2D& minI, const Vector2D& maxI) const {
    const bool xHit = minI.X < maxV.X && maxI.X > minV.X;
    const bool yHit = minI.Y < maxV.Y && maxI.Y > minV.Y;
    return xHit && yHit;
}

bool koilo::Rectangle2D::Contains(const Vector2D& v) const {
    return minV.X <= v.X && v.X <= maxV.X && minV.Y <= v.Y && v.Y <= maxV.Y;
}

bool koilo::Rectangle2D::OverlapsCircle(const Circle2D& c) const {
    Vector2D cc = c.GetCenter();
    Bounds b = GetBounds();
    float cx = std::max(b.minV.X, std::min(cc.X, b.maxV.X));
    float cy = std::max(b.minV.Y, std::min(cc.Y, b.maxV.Y));
    float dx = cc.X - cx;
    float dy = cc.Y - cy;
    float r = c.GetRadius();
    return (dx * dx + dy * dy) <= (r * r);
}

} // namespace koilo
