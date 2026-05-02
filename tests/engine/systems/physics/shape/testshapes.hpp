// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <unity.h>
#include <koilo/systems/physics/shape/sphereshape.hpp>
#include <koilo/systems/physics/shape/boxshape.hpp>
#include <koilo/systems/physics/shape/capsuleshape.hpp>
#include <koilo/systems/physics/shape/convexhullshape.hpp>
#include <koilo/systems/physics/shape/planeshape.hpp>
#include <koilo/systems/physics/colliderproxy.hpp>
#include <utils/testhelpers.hpp>

class TestShapes {
public:
    // Sphere
    static void TestSphereLocalAABB();
    static void TestSphereWorldAABBTranslated();
    static void TestSphereSupport();

    // Box
    static void TestBoxLocalAABB();
    static void TestBoxWorldAABBRotated();
    static void TestBoxSupportAxisAligned();
    static void TestBoxSupportRotated();

    // Capsule
    static void TestCapsuleLocalAABB();
    static void TestCapsuleSupportAxis();

    // ConvexHull shape
    static void TestConvexHullSupportTransformed();

    // Plane
    static void TestPlaneIsNotConvex();
    static void TestPlaneNormalWorld();

    // Proxy
    static void TestColliderProxyRefresh();
    static void TestColliderProxyNoShape();

    static void RunAllTests() {
        RUN_TEST(TestSphereLocalAABB);
        RUN_TEST(TestSphereWorldAABBTranslated);
        RUN_TEST(TestSphereSupport);
        RUN_TEST(TestBoxLocalAABB);
        RUN_TEST(TestBoxWorldAABBRotated);
        RUN_TEST(TestBoxSupportAxisAligned);
        RUN_TEST(TestBoxSupportRotated);
        RUN_TEST(TestCapsuleLocalAABB);
        RUN_TEST(TestCapsuleSupportAxis);
        RUN_TEST(TestConvexHullSupportTransformed);
        RUN_TEST(TestPlaneIsNotConvex);
        RUN_TEST(TestPlaneNormalWorld);
        RUN_TEST(TestColliderProxyRefresh);
        RUN_TEST(TestColliderProxyNoShape);
    }
};
