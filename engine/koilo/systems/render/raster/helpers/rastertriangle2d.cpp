// SPDX-License-Identifier: GPL-3.0-or-later
#include <koilo/systems/render/raster/helpers/rastertriangle2d.hpp>


namespace koilo {

/**
 * @file rastertriangle2d.cpp
 * @brief Flat rasterizable triangle - projects 3D data into screen space.
 */

koilo::RasterTriangle2D::RasterTriangle2D()
    : p1(), p2(), p3(),
      wp1(), wp2(), wp3(), normal(),
      material(nullptr), shadeFn(nullptr), shaderInstance(nullptr), frameCtx(nullptr),
      uv1(), uv2(), uv3(),
      z1(0.0f), z2(0.0f), z3(0.0f),
      denominator(0.0f),
      boundsMinX(0.0f), boundsMinY(0.0f), boundsMaxX(1.0f), boundsMaxY(1.0f),
      pixMinX(0), pixMinY(0), pixMaxX(0), pixMaxY(0),
      averageDepth(0.0f) {}

koilo::RasterTriangle2D::RasterTriangle2D(const Transform& camTransform, const Quaternion& lookDirection,
                                   const RasterTriangle3D& sourceTriangle, IMaterial* mat)
{
    this->material = mat;
    this->wp1 = *sourceTriangle.p1;
    this->wp2 = *sourceTriangle.p2;
    this->wp3 = *sourceTriangle.p3;
    this->normal = sourceTriangle.normal;

    if (sourceTriangle.hasUV) {
        this->uv1 = *sourceTriangle.uv1;
        this->uv2 = *sourceTriangle.uv2;
        this->uv3 = *sourceTriangle.uv3;
    }

    Quaternion invRot = lookDirection.Conjugate();
    float qw = invRot.W, qx = invRot.X, qy = invRot.Y, qz = invRot.Z;
    Vector3D camPos = camTransform.GetPosition();
    Vector3D camScl = camTransform.GetScale();
    float invSx = (camScl.X != 0.0f) ? 1.0f / camScl.X : 0.0f;
    float invSy = (camScl.Y != 0.0f) ? 1.0f / camScl.Y : 0.0f;
    float invSz = (camScl.Z != 0.0f) ? 1.0f / camScl.Z : 0.0f;

    auto rotVert = [&](const Vector3D& world, float& ox, float& oy, float& oz) {
        float rx = world.X - camPos.X;
        float ry = world.Y - camPos.Y;
        float rz = world.Z - camPos.Z;
        float tx = 2.0f * (qy * rz - qz * ry);
        float ty = 2.0f * (qz * rx - qx * rz);
        float tz = 2.0f * (qx * ry - qy * rx);
        ox = (rx + qw * tx + (qy * tz - qz * ty)) * invSx;
        oy = (ry + qw * ty + (qz * tx - qx * tz)) * invSy;
        oz = (rz + qw * tz + (qx * ty - qy * tx)) * invSz;
    };

    float px1, py1, pz1, px2, py2, pz2, px3, py3, pz3;
    rotVert(wp1, px1, py1, pz1);
    rotVert(wp2, px2, py2, pz2);
    rotVert(wp3, px3, py3, pz3);

    this->z1 = pz1;
    this->z2 = pz2;
    this->z3 = pz3;

    this->p1 = Vector2D(px1, py1);
    this->p2 = Vector2D(px2, py2);
    this->p3 = Vector2D(px3, py3);

    this->averageDepth = (pz1 + pz2 + pz3) * (1.0f / 3.0f);

    CalculateBoundsAndDenominator();
}

koilo::RasterTriangle2D::RasterTriangle2D(const Transform& camTransform, const Quaternion& lookDirection,
                                   const RasterTriangle3D& sourceTriangle, IMaterial* mat,
                                   float nearPlane, float fovScale,
                                   float viewportCenterX, float viewportCenterY,
                                   float viewportHalfW, float viewportHalfH)
{
    this->material = mat;
    this->wp1 = *sourceTriangle.p1;
    this->wp2 = *sourceTriangle.p2;
    this->wp3 = *sourceTriangle.p3;
    this->normal = sourceTriangle.normal;

    if (sourceTriangle.hasUV) {
        this->uv1 = *sourceTriangle.uv1;
        this->uv2 = *sourceTriangle.uv2;
        this->uv3 = *sourceTriangle.uv3;
    }

    Quaternion invRot = lookDirection.Conjugate();
    float qw = invRot.W, qx = invRot.X, qy = invRot.Y, qz = invRot.Z;
    Vector3D camPos = camTransform.GetPosition();
    Vector3D camScl = camTransform.GetScale();
    float invSx = (camScl.X != 0.0f) ? 1.0f / camScl.X : 0.0f;
    float invSy = (camScl.Y != 0.0f) ? 1.0f / camScl.Y : 0.0f;
    float invSz = (camScl.Z != 0.0f) ? 1.0f / camScl.Z : 0.0f;
    float clampNear = nearPlane > 0.001f ? nearPlane : 0.001f;

    auto rotProject = [&](const Vector3D& world, float& outSx, float& outSy, float& outZ) {
        float rx = world.X - camPos.X;
        float ry = world.Y - camPos.Y;
        float rz = world.Z - camPos.Z;
        float tx = 2.0f * (qy * rz - qz * ry);
        float ty = 2.0f * (qz * rx - qx * rz);
        float tz = 2.0f * (qx * ry - qy * rx);
        float px = (rx + qw * tx + (qy * tz - qz * ty)) * invSx;
        float py = (ry + qw * ty + (qz * tx - qx * tz)) * invSy;
        float pz = (rz + qw * tz + (qx * ty - qy * tx)) * invSz;
        outZ = -pz;
        float dz = (outZ > clampNear) ? outZ : clampNear;
        float invDz = fovScale * viewportHalfH / dz;
        outSx = viewportCenterX + px * invDz;
        outSy = viewportCenterY - py * invDz;
    };

    float sx1, sy1, sx2, sy2, sx3, sy3;
    rotProject(wp1, sx1, sy1, z1);
    rotProject(wp2, sx2, sy2, z2);
    rotProject(wp3, sx3, sy3, z3);

    this->p1 = Vector2D(sx1, sy1);
    this->p2 = Vector2D(sx2, sy2);
    this->p3 = Vector2D(sx3, sy3);

    this->averageDepth = (z1 + z2 + z3) * (1.0f / 3.0f);

    CalculateBoundsAndDenominator();
}

koilo::RasterTriangle2D::RasterTriangle2D(const Transform& camTransform, const Quaternion& lookDirection,
                                   const Vector3D* v1, const Vector3D* v2, const Vector3D* v3,
                                   const Vector2D* t1, const Vector2D* t2, const Vector2D* t3,
                                   IMaterial* mat)
{
    this->material = mat;
    this->wp1 = *v1;
    this->wp2 = *v2;
    this->wp3 = *v3;

    // Compute normal inline - skip UnitSphere(), shaders normalize themselves
    Vector3D e1 = *v2 - *v1;
    Vector3D e2 = *v3 - *v1;
    this->normal = e1.CrossProduct(e2);

    if (t1) {
        this->uv1 = *t1;
        this->uv2 = *t2;
        this->uv3 = *t3;
    }

    Quaternion invRot = lookDirection.Conjugate();
    float qw = invRot.W, qx = invRot.X, qy = invRot.Y, qz = invRot.Z;
    Vector3D camPos = camTransform.GetPosition();
    Vector3D camScl = camTransform.GetScale();
    float invSx = (camScl.X != 0.0f) ? 1.0f / camScl.X : 0.0f;
    float invSy = (camScl.Y != 0.0f) ? 1.0f / camScl.Y : 0.0f;
    float invSz = (camScl.Z != 0.0f) ? 1.0f / camScl.Z : 0.0f;

    auto rotVert = [&](const Vector3D& world, float& ox, float& oy, float& oz) {
        float rx = world.X - camPos.X;
        float ry = world.Y - camPos.Y;
        float rz = world.Z - camPos.Z;
        float tx = 2.0f * (qy * rz - qz * ry);
        float ty = 2.0f * (qz * rx - qx * rz);
        float tz = 2.0f * (qx * ry - qy * rx);
        ox = (rx + qw * tx + (qy * tz - qz * ty)) * invSx;
        oy = (ry + qw * ty + (qz * tx - qx * tz)) * invSy;
        oz = (rz + qw * tz + (qx * ty - qy * tx)) * invSz;
    };

    float px1, py1, pz1, px2, py2, pz2, px3, py3, pz3;
    rotVert(wp1, px1, py1, pz1);
    rotVert(wp2, px2, py2, pz2);
    rotVert(wp3, px3, py3, pz3);

    this->z1 = pz1;
    this->z2 = pz2;
    this->z3 = pz3;

    this->p1 = Vector2D(px1, py1);
    this->p2 = Vector2D(px2, py2);
    this->p3 = Vector2D(px3, py3);

    this->averageDepth = (pz1 + pz2 + pz3) * (1.0f / 3.0f);

    CalculateBoundsAndDenominator();
}

koilo::RasterTriangle2D::RasterTriangle2D(const Transform& camTransform, const Quaternion& lookDirection,
                                   const Vector3D* v1, const Vector3D* v2, const Vector3D* v3,
                                   const Vector2D* t1, const Vector2D* t2, const Vector2D* t3,
                                   IMaterial* mat,
                                   float nearPlane, float fovScale,
                                   float viewportCenterX, float viewportCenterY,
                                   float viewportHalfW, float viewportHalfH)
{
    this->material = mat;
    this->wp1 = *v1;
    this->wp2 = *v2;
    this->wp3 = *v3;

    Vector3D e1 = *v2 - *v1;
    Vector3D e2 = *v3 - *v1;
    this->normal = e1.CrossProduct(e2);

    if (t1) {
        this->uv1 = *t1;
        this->uv2 = *t2;
        this->uv3 = *t3;
    }

    Quaternion invRot = lookDirection.Conjugate();
    float qw = invRot.W, qx = invRot.X, qy = invRot.Y, qz = invRot.Z;
    Vector3D camPos = camTransform.GetPosition();
    Vector3D camScl = camTransform.GetScale();
    float invSx = (camScl.X != 0.0f) ? 1.0f / camScl.X : 0.0f;
    float invSy = (camScl.Y != 0.0f) ? 1.0f / camScl.Y : 0.0f;
    float invSz = (camScl.Z != 0.0f) ? 1.0f / camScl.Z : 0.0f;
    float clampNear = nearPlane > 0.001f ? nearPlane : 0.001f;

    auto rotProject = [&](const Vector3D& world, float& outSx, float& outSy, float& outZ) {
        float rx = world.X - camPos.X;
        float ry = world.Y - camPos.Y;
        float rz = world.Z - camPos.Z;
        float tx = 2.0f * (qy * rz - qz * ry);
        float ty = 2.0f * (qz * rx - qx * rz);
        float tz = 2.0f * (qx * ry - qy * rx);
        float px = (rx + qw * tx + (qy * tz - qz * ty)) * invSx;
        float py = (ry + qw * ty + (qz * tx - qx * tz)) * invSy;
        float pz = (rz + qw * tz + (qx * ty - qy * tx)) * invSz;
        outZ = -pz;
        float dz = (outZ > clampNear) ? outZ : clampNear;
        float invDz = fovScale * viewportHalfH / dz;
        outSx = viewportCenterX + px * invDz;
        outSy = viewportCenterY - py * invDz;
    };

    float sx1, sy1, sx2, sy2, sx3, sy3;
    rotProject(wp1, sx1, sy1, z1);
    rotProject(wp2, sx2, sy2, z2);
    rotProject(wp3, sx3, sy3, z3);

    this->p1 = Vector2D(sx1, sy1);
    this->p2 = Vector2D(sx2, sy2);
    this->p3 = Vector2D(sx3, sy3);

    this->averageDepth = (z1 + z2 + z3) * (1.0f / 3.0f);

    CalculateBoundsAndDenominator();
}

bool koilo::RasterTriangle2D::IsBackFacing() const {
    float edge1x = p2.X - p1.X;
    float edge1y = p2.Y - p1.Y;
    float edge2x = p3.X - p1.X;
    float edge2y = p3.Y - p1.Y;
    return (edge1x * edge2y - edge1y * edge2x) > 0.0f;
}

void koilo::RasterTriangle2D::CalculateBoundsAndDenominator() {
    v0 = p2 - p1;
    v1 = p3 - p1;

    denominator = (v0.X * v1.Y - v1.X * v0.Y);
    if (Mathematics::FAbs(denominator) > Mathematics::EPSILON) {
        denominator = 1.0f / denominator;
    } else {
        denominator = 0.0f;
    }

    boundsMinX = Mathematics::Min(p1.X, p2.X, p3.X);
    boundsMinY = Mathematics::Min(p1.Y, p2.Y, p3.Y);
    boundsMaxX = Mathematics::Max(p1.X, p2.X, p3.X);
    boundsMaxY = Mathematics::Max(p1.Y, p2.Y, p3.Y);
}

bool koilo::RasterTriangle2D::GetBarycentricCoords(float x, float y, float& u, float& v, float& w) const {
    if (denominator == 0.0f) return false;

    Vector2D v2 = Vector2D(x, y) - p1;

    v = (v2.X * v1.Y - v1.X * v2.Y) * denominator;
    w = (v0.X * v2.Y - v2.X * v0.Y) * denominator;
    u = 1.0f - v - w;

    return (v >= 0.0f) && (w >= 0.0f) && (u >= 0.0f);
}

bool koilo::RasterTriangle2D::Overlaps(float oMinX, float oMinY, float oMaxX, float oMaxY) const {
    return boundsMaxX >= oMinX && boundsMinX <= oMaxX &&
           boundsMaxY >= oMinY && boundsMinY <= oMaxY;
}

IMaterial* koilo::RasterTriangle2D::GetMaterial() const {
    return material;
}

koilo::UString koilo::RasterTriangle2D::ToString() const {
    return p1.ToString() + " " + p2.ToString() + " " + p3.ToString();
}

} // namespace koilo
