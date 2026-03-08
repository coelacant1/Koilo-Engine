#include <ptx/core/geometry/2d/rectangle.hpp>

Rectangle2D::Rectangle2D(Vector2D center, Vector2D size, float rotation) : Shape(center, size, rotation) {}

Rectangle2D::Rectangle2D(Bounds bounds, float rotationDeg) : Shape(bounds, rotationDeg) {}

bool Rectangle2D::IsInShape(Vector2D p) {
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

Rectangle2D::Corners Rectangle2D::GetCorners() const {
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

void Rectangle2D::UpdateBounds(const Vector2D& v){
    minV = minV.Minimum(v);
    maxV = maxV.Maximum(v);
    midV = (minV + maxV) * 0.5f;
}

Vector2D Rectangle2D::GetMinimum() const {
    return minV; 
}

Vector2D Rectangle2D::GetMaximum() const {
    return maxV;
}

Vector2D Rectangle2D::GetCenter()  const {
    return midV;
}

bool Rectangle2D::Overlaps(const Rectangle2D& other) const{
    return Overlaps(other.minV, other.maxV);
}

bool Rectangle2D::Overlaps(const Vector2D& minI, const Vector2D& maxI) const {
    const bool xHit = minI.X < maxV.X && maxI.X > minV.X;
    const bool yHit = minI.Y < maxV.Y && maxI.Y > minV.Y;
    return xHit && yHit;
}

bool Rectangle2D::Contains(const Vector2D& v) const {
    return minV.X <= v.X && v.X <= maxV.X && minV.Y <= v.Y && v.Y <= maxV.Y;
}
