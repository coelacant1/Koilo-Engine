#include "../../../../include/um3d/render/raster/helpers/rastertriangle2d.hpp"

static inline float Edge(const Vector2D& a,
                         const Vector2D& b,
                         float           px,
                         float           py) {
    return (px - a.x) * (b.y - a.y) - (py - a.y) * (b.x - a.x);
}

RasterTriangle2D::RasterTriangle2D(const Triangle2D& t)
    : tri(t), uv1(nullptr), uv2(nullptr), uv3(nullptr),
      nrm(nullptr), mat(nullptr), avgDepth(0.0f), hasUV(false) {
    v0x = tri.p2.x - tri.p1.x;
    v0y = tri.p2.y - tri.p1.y;
    v1x = tri.p3.x - tri.p1.x;
    v1y = tri.p3.y - tri.p1.y;
    denom = v0x * v1y - v1x * v0y;
}

RasterTriangle2D::RasterTriangle2D(const Triangle2D& t,
                                   const Vector2D* uv1In,
                                   const Vector2D* uv2In,
                                   const Vector2D* uv3In,
                                   const Vector3D* nrmIn,
                                   Material*       matIn,
                                   float           depthIn): tri(t), uv1(uv1In), uv2(uv2In), uv3(uv3In),
      nrm(nrmIn), mat(matIn), avgDepth(depthIn), hasUV(true) {
    v0x = tri.p2.x - tri.p1.x;
    v0y = tri.p2.y - tri.p1.y;
    v1x = tri.p3.x - tri.p1.x;
    v1y = tri.p3.y - tri.p1.y;
    denom = v0x * v1y - v1x * v0y;
}

bool RasterTriangle2D::Hit(float x, float y, float& u, float& v, float& w) const {
    float e0 = Edge(tri.p1, tri.p2, x, y);
    float e1 = Edge(tri.p2, tri.p3, x, y);
    float e2 = Edge(tri.p3, tri.p1, x, y);

    if ((e0 < 0.0f) || (e1 < 0.0f) || (e2 < 0.0f))
        return false;

    v = e0 / denom;
    w = e1 / denom;
    u = 1.0f - v - w;
    return true;
}
