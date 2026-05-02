// SPDX-License-Identifier: GPL-3.0-or-later
#include "testshapes.hpp"
#include <cmath>

using namespace koilo;

namespace {
Quaternion RotZ(float radians) {
    const float h = radians * 0.5f;
    return Quaternion(std::cos(h), 0.0f, 0.0f, std::sin(h));
}
}

// ===== Sphere =====
void TestShapes::TestSphereLocalAABB() {
    SphereShape s(2.0f);
    AABB a = s.LocalAABB();
    TEST_ASSERT_FLOAT_WITHIN(1e-5, -2.0, a.min.X);
    TEST_ASSERT_FLOAT_WITHIN(1e-5,  2.0, a.max.Z);
    TEST_ASSERT_EQUAL(static_cast<int>(ShapeType::Sphere), static_cast<int>(s.Type()));
    TEST_ASSERT_TRUE(s.IsConvex());
}

void TestShapes::TestSphereWorldAABBTranslated() {
    SphereShape s(1.0f);
    BodyPose pose(Vector3D(5,0,0), Quaternion());
    AABB a = s.WorldAABB(pose);
    TEST_ASSERT_FLOAT_WITHIN(1e-5, 4.0, a.min.X);
    TEST_ASSERT_FLOAT_WITHIN(1e-5, 6.0, a.max.X);
}

void TestShapes::TestSphereSupport() {
    SphereShape s(2.0f);
    BodyPose pose(Vector3D(1,0,0), Quaternion());
    Vector3D sup = s.Support(Vector3D(1,0,0), pose);
    TEST_ASSERT_FLOAT_WITHIN(1e-5, 3.0, sup.X);
    TEST_ASSERT_FLOAT_WITHIN(1e-5, 0.0, sup.Y);
}

// ===== Box =====
void TestShapes::TestBoxLocalAABB() {
    BoxShape b(Vector3D(1, 2, 3));
    AABB a = b.LocalAABB();
    TEST_ASSERT_FLOAT_WITHIN(1e-5, -1.0, a.min.X);
    TEST_ASSERT_FLOAT_WITHIN(1e-5,  3.0, a.max.Z);
}

void TestShapes::TestBoxWorldAABBRotated() {
    BoxShape b(Vector3D(1,1,1));
    BodyPose pose(Vector3D(0,0,0), RotZ(3.14159265f * 0.25f));
    AABB a = b.WorldAABB(pose);
    TEST_ASSERT_FLOAT_WITHIN(1e-3, std::sqrt(2.0f), a.max.X);
}

void TestShapes::TestBoxSupportAxisAligned() {
    BoxShape b(Vector3D(1,1,1));
    BodyPose pose;
    Vector3D sup = b.Support(Vector3D(1,0,0), pose);
    TEST_ASSERT_FLOAT_WITHIN(1e-5, 1.0, sup.X);
    TEST_ASSERT_FLOAT_WITHIN(1e-5, 1.0, sup.Y); // sign(0)>=0 picks positive face
}

void TestShapes::TestBoxSupportRotated() {
    BoxShape b(Vector3D(1,1,1));
    BodyPose pose(Vector3D(0,0,0), RotZ(3.14159265f * 0.25f));
    Vector3D sup = b.Support(Vector3D(1,0,0), pose);
    TEST_ASSERT_FLOAT_WITHIN(1e-3, std::sqrt(2.0f), sup.X);
    TEST_ASSERT_FLOAT_WITHIN(1e-3, 0.0, sup.Y);
}

// ===== Capsule =====
void TestShapes::TestCapsuleLocalAABB() {
    CapsuleShape c(0.5f, 1.0f);
    AABB a = c.LocalAABB();
    TEST_ASSERT_FLOAT_WITHIN(1e-5, -0.5, a.min.X);
    TEST_ASSERT_FLOAT_WITHIN(1e-5, -1.5, a.min.Y);
    TEST_ASSERT_FLOAT_WITHIN(1e-5,  1.5, a.max.Y);
}

void TestShapes::TestCapsuleSupportAxis() {
    CapsuleShape c(0.5f, 1.0f);
    BodyPose pose;
    Vector3D top = c.Support(Vector3D(0,1,0), pose);
    TEST_ASSERT_FLOAT_WITHIN(1e-5, 1.5, top.Y);
    Vector3D side = c.Support(Vector3D(1,0,0), pose);
    TEST_ASSERT_FLOAT_WITHIN(1e-5, 0.5, side.X);
}

// ===== ConvexHull shape =====
void TestShapes::TestConvexHullSupportTransformed() {
    ConvexHull h({
        Vector3D( 1, 0, 0),
        Vector3D(-1, 0, 0),
        Vector3D( 0, 1, 0),
        Vector3D( 0, 0, 1)
    });
    ConvexHullShape s(std::move(h));
    BodyPose pose(Vector3D(10, 0, 0), RotZ(3.14159265f * 0.5f)); // 90° about Z
    // Local +X -> world +Y. Asking world +Y should return rotated (1,0,0) translated.
    Vector3D sup = s.Support(Vector3D(0,1,0), pose);
    TEST_ASSERT_FLOAT_WITHIN(1e-3, 10.0, sup.X);
    TEST_ASSERT_FLOAT_WITHIN(1e-3,  1.0, sup.Y);
}

// ===== Plane =====
void TestShapes::TestPlaneIsNotConvex() {
    PlaneShape p(Vector3D(0,1,0), 0.0f);
    TEST_ASSERT_FALSE(p.IsConvex());
    TEST_ASSERT_EQUAL(static_cast<int>(ShapeType::Plane), static_cast<int>(p.Type()));
}

void TestShapes::TestPlaneNormalWorld() {
    PlaneShape p(Vector3D(0,1,0), 0.0f);
    BodyPose pose(Vector3D(0,0,0), RotZ(3.14159265f * 0.5f));
    Vector3D n = p.NormalWorld(pose);
    TEST_ASSERT_FLOAT_WITHIN(1e-3, -1.0, n.X);
    TEST_ASSERT_FLOAT_WITHIN(1e-3,  0.0, n.Y);
}

// ===== Proxy =====
void TestShapes::TestColliderProxyRefresh() {
    SphereShape s(1.0f);
    ColliderProxy proxy;
    proxy.shape = &s;
    proxy.localOffset = BodyPose(Vector3D(2,0,0));
    BodyPose body(Vector3D(10,0,0), Quaternion());
    proxy.RefreshWorldAABB(body);
    // Body at (10,0,0), offset (2,0,0) -> sphere center (12,0,0) ± 1
    TEST_ASSERT_FLOAT_WITHIN(1e-5, 11.0, proxy.worldAabb.min.X);
    TEST_ASSERT_FLOAT_WITHIN(1e-5, 13.0, proxy.worldAabb.max.X);
}

void TestShapes::TestColliderProxyNoShape() {
    ColliderProxy proxy;
    BodyPose body(Vector3D(7,0,0));
    proxy.RefreshWorldAABB(body); // must not crash
    TEST_ASSERT_FLOAT_WITHIN(1e-5, 7.0, proxy.worldAabb.min.X);
    TEST_ASSERT_FLOAT_WITHIN(1e-5, 7.0, proxy.worldAabb.max.X);
    TEST_ASSERT_EQUAL(ColliderProxy::kInvalidHandle, proxy.broadphaseHandle);
}
