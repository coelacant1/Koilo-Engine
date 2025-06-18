#include "../../../../include/um3d/render/raster/helpers/rastertriangle3d.hpp"

RasterTriangle3D::RasterTriangle3D(const Triangle3D& g,
                                   float z1, float z2, float z3,
                                   Material* m)
    : geo(g), oneOverZ1(z1), oneOverZ2(z2), oneOverZ3(z3), mat(m) {
    /* plane equation a·x + b·y + c = 1/z */
    float x1 = g.p1.x, y1 = g.p1.y;
    float x2 = g.p2.x, y2 = g.p2.y;
    float x3 = g.p3.x, y3 = g.p3.y;

    float det = (x2 - x1) * (y3 - y1) - (x3 - x1) * (y2 - y1);
    float invDet = (det == 0.0f) ? 0.0f : 1.0f / det;

    a = ((oneOverZ2 - oneOverZ1) * (y3 - y1) -
         (oneOverZ3 - oneOverZ1) * (y2 - y1)) * invDet;

    b = ((x2 - x1) * (oneOverZ3 - oneOverZ1) -
         (x3 - x1) * (oneOverZ2 - oneOverZ1)) * invDet;

    c = oneOverZ1 - a * x1 - b * y1;
}

float RasterTriangle3D::DepthAt(float x, float y) const {
    return a * x + b * y + c;
}
