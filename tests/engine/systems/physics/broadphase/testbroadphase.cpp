// SPDX-License-Identifier: GPL-3.0-or-later
#include "testbroadphase.hpp"
#include <koilo/systems/physics/broadphase/broadphase.hpp>
#include <koilo/systems/physics/colliderproxy.hpp>
#include <koilo/systems/physics/shape/sphereshape.hpp>
#include <koilo/systems/physics/shape/boxshape.hpp>
#include <koilo/systems/physics/shape/planeshape.hpp>
#include <utils/testhelpers.hpp>

using namespace koilo;

namespace {
struct Fixture {
    SphereShape sphere{0.5f};
    BoxShape    box{Vector3D(0.5f, 0.5f, 0.5f)};
    PlaneShape  plane{Vector3D(0,1,0), 0.0f};
    ColliderProxy a, b, c, p;
    Fixture() {
        a.shape = &sphere;
        b.shape = &sphere;
        c.shape = &box;
        p.shape = &plane;
    }
};
}

void TestBroadphase::TestAddAssignsMonotonicProxyId() {
    Broadphase bp;
    Fixture f;
    bp.Add(&f.a, BodyPose(Vector3D(0,0,0)));
    bp.Add(&f.b, BodyPose(Vector3D(2,0,0)));
    bp.Add(&f.c, BodyPose(Vector3D(4,0,0)));
    TEST_ASSERT_EQUAL_UINT32(0u, f.a.proxyId);
    TEST_ASSERT_EQUAL_UINT32(1u, f.b.proxyId);
    TEST_ASSERT_EQUAL_UINT32(2u, f.c.proxyId);
}

void TestBroadphase::TestPlanesGoToPlaneRegistryNotTree() {
    Broadphase bp;
    Fixture f;
    bp.Add(&f.p, BodyPose(Vector3D(0,0,0)));
    TEST_ASSERT_EQUAL_size_t(1u, bp.PlaneCount());
    TEST_ASSERT_EQUAL_size_t(0u, bp.TreeNodeCount());
}

void TestBroadphase::TestPairsSortedByProxyId() {
    Broadphase bp;
    Fixture f;
    bp.Add(&f.a, BodyPose(Vector3D(0,0,0)));
    bp.Add(&f.b, BodyPose(Vector3D(0.5f,0,0)));
    bp.Add(&f.c, BodyPose(Vector3D(0.5f,0.5f,0)));
    auto pairs = bp.CollectPairs();
    TEST_ASSERT_TRUE(pairs.size() >= 1u);
    for (auto& pr : pairs) {
        TEST_ASSERT_TRUE(pr.first->proxyId < pr.second->proxyId);
    }
}

void TestBroadphase::TestPlaneVsBoxPairProduced() {
    Broadphase bp;
    Fixture f;
    bp.Add(&f.c, BodyPose(Vector3D(0,0,0)));     // box at origin
    bp.Add(&f.p, BodyPose(Vector3D(0,0,0)));     // plane at origin
    auto pairs = bp.CollectPairs();
    TEST_ASSERT_EQUAL_size_t(1u, pairs.size());
    // Plane was added second, so plane.proxyId > box.proxyId.
    TEST_ASSERT_EQUAL_PTR(&f.c, pairs[0].first);
    TEST_ASSERT_EQUAL_PTR(&f.p, pairs[0].second);
}

void TestBroadphase::TestRemoveDropsPair() {
    Broadphase bp;
    Fixture f;
    bp.Add(&f.a, BodyPose(Vector3D(0,0,0)));
    bp.Add(&f.b, BodyPose(Vector3D(0.2f,0,0)));
    TEST_ASSERT_EQUAL_size_t(1u, bp.CollectPairs().size());
    bp.Remove(&f.b);
    TEST_ASSERT_EQUAL_size_t(0u, bp.CollectPairs().size());
}

void TestBroadphase::TestUpdateRefreshesAabb() {
    Broadphase bp;
    Fixture f;
    bp.Add(&f.a, BodyPose(Vector3D(0,0,0)));
    bp.Add(&f.b, BodyPose(Vector3D(10,0,0)));
    TEST_ASSERT_EQUAL_size_t(0u, bp.CollectPairs().size());
    bp.Update(&f.b, BodyPose(Vector3D(0.2f,0,0)));
    TEST_ASSERT_EQUAL_size_t(1u, bp.CollectPairs().size());
}

void TestBroadphase::RunAllTests() {
    RUN_TEST(TestAddAssignsMonotonicProxyId);
    RUN_TEST(TestPlanesGoToPlaneRegistryNotTree);
    RUN_TEST(TestPairsSortedByProxyId);
    RUN_TEST(TestPlaneVsBoxPairProduced);
    RUN_TEST(TestRemoveDropsPair);
    RUN_TEST(TestUpdateRefreshesAabb);
}
