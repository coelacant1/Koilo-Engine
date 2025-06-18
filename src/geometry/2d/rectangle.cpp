#include "../../../include/um3d/geometry/2d/rectangle.hpp"

Rectangle2D::Rectangle2D(Vector2D center, Vector2D size, float rotation) : Shape(center, size, rotation) {}

bool Rectangle2D::IsInShape(Vector2D p) const{
    const float dx = p.X - center.X;
    const float dy = p.Y - center.Y;

    const float sinR = std::sin(rotation * Mathematics::MPID180);
    const float cosR = std::cos(rotation * Mathematics::MPID180);

    const float xLocal =  dx * cosR - dy * sinR;
    const float yLocal =  dx * sinR + dy * cosR;

    return Mathematics::FAbs(xLocal) <= size.X * 0.5f && Mathematics::FAbs(yLocal) <= size.Y * 0.5f;
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
