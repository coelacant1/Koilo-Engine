#include "overlap.hpp"

bool Overlap2D::Overlaps(const Rectangle2D& a, const Rectangle2D& b){
    if (Mathematics::IsClose(a.GetRotation(), 0.0f, 0.05f), Mathematics::IsClose(b.GetRotation(), 0.0f, 0.05f)){

        Shape::Bounds ba = a.GetBounds();
        Shape::Bounds bb = b.GetBounds();

        return !(ba.maxV.X < bb.minV.X || ba.minV.X > bb.maxV.X ||
                ba.maxV.Y < bb.minV.Y || ba.minV.Y > bb.maxV.Y);
    }
    else {
        // gather corners in world space
        const Rectangle2D::Corners ca = a.GetCorners();
        const Rectangle2D::Corners cb = b.GetCorners();

        // two edge normals per rectangle
        Vector2D axes[4] = {
            (ca.corners[1] - ca.corners[0]).Perpendicular(), // edge 0-1 normal (rect a)
            (ca.corners[3] - ca.corners[0]).Perpendicular(), // edge 0-3 normal (rect a)
            (cb.corners[1] - cb.corners[0]).Perpendicular(), // edge 0-1 normal (rect b)
            (cb.corners[3] - cb.corners[0]).Perpendicular()  // edge 0-3 normal (rect b)
        };

        for (unsigned char k = 0; k < 4; ++k) {
            Vector2D n = axes[k]; // separating axis

            // project rectangle a
            float minA = ca.corners[0].DotProduct(n);
            float maxA = minA;
            
            for (unsigned char i = 1; i < 4; ++i) {
                float p = ca.corners[i].DotProduct(n);

                if (p < minA) minA = p;
                if (p > maxA) maxA = p;
            }

            // project rectangle b
            float minB = cb.corners[0].DotProduct(n);
            float maxB = minB;

            for (unsigned char i = 1; i < 4; ++i) {
                float p = cb.corners[i].DotProduct(n);

                if (p < minB) minB = p;
                if (p > maxB) maxB = p;
            }

            // gap on this axis? -> no overlap
            if (maxA < minB || maxB < minA) {
                return false; // early exit
            }
        }

        // projections overlap on every axis
        return true;
    }
}

bool Overlap2D::Overlaps(const Circle2D& a, const Circle2D& b){
    Vector2D ca = a.GetCenter();
    Vector2D cb = b.GetCenter();

    float dx = ca.X - cb.X;
    float dy = ca.Y - cb.Y;
    float r  = (a.GetSize().X + b.GetSize().X) / 4.0f;

    return (dx * dx + dy * dy) <= r * r;
}

// Needs finished
bool Overlap2D::Overlaps(const Ellipse2D& a, const Ellipse2D& b){
    return Overlaps(Rectangle2D(a.GetBounds()), Rectangle2D(b.GetBounds()));
}

// Needs finished
bool Overlap2D::Overlaps(const Triangle2D& a, const Triangle2D& b){
    return Overlaps(Rectangle2D(a.GetBounds()), Rectangle2D(b.GetBounds()));
}

bool Overlap2D::Overlaps(const Rectangle2D& r, const Circle2D& c){
    Shape::Bounds br = r.GetBounds();
    Shape::Bounds bc = c.GetBounds();
    Vector2D cc = c.GetCenter();

    float cx = fmaxf(br.minV.X, fminf(cc.X, br.maxV.X));
    float cy = fmaxf(br.minV.Y, fminf(cc.Y, br.maxV.Y));
    float dx = cc.X - cx;
    float dy = cc.Y - cy;

    return (dx * dx + dy * dy) <= (c.GetSize().X / 2.0f) * (c.GetSize().X / 2.0f);
}

// Needs finished 
bool Overlap2D::Overlaps(const Rectangle2D& r, const Triangle2D& t){
    return Overlaps(r, Rectangle2D(t.GetBounds()));
}

bool Overlap2D::Overlaps(const Rectangle2D& r, const Ellipse2D& e){
    return Overlaps(r, Rectangle2D(e.GetBounds()));
}

bool Overlap2D::Overlaps(const Circle2D& c, const Triangle2D& t){
    return Overlaps(Rectangle2D(t.GetBounds()), c);
}

bool Overlap2D::Overlaps(const Circle2D& c, const Ellipse2D& e){
    return Overlaps(Rectangle2D(e.GetBounds()), c);
}

bool Overlap2D::Overlaps(const Triangle2D& t, const Ellipse2D& e){
    return Overlaps(Rectangle2D(t.GetBounds()), Rectangle2D(e.GetBounds()));
}
