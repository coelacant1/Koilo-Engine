// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testphysicsworld.cpp
 * @brief Implementation of PhysicsWorld unit tests.
 */

#include "testphysicsworld.hpp"
#include <koilo/systems/physics/spherecollider.hpp>
#include <koilo/systems/physics/capsulecollider.hpp>
#include <koilo/systems/physics/boxcollider.hpp>
#include <koilo/systems/physics/meshcollider.hpp>
#include <koilo/systems/physics/heightfieldcollider.hpp>
#include <koilo/systems/physics/shape/trianglemeshdata.hpp>
#include <koilo/systems/physics/shape/heightfielddata.hpp>
#include <koilo/systems/physics/raycasthit.hpp>
#include <koilo/core/geometry/ray.hpp>
#include <cmath>
#include <memory>

using namespace koilo;

// ========== Constructor Tests ==========

void TestPhysicsWorld::TestDefaultConstructor() {
    PhysicsWorld obj;
    TEST_ASSERT_EQUAL(0, obj.GetBodyCount());
    Vector3D g = obj.GetGravity();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, g.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -9.81f, g.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, g.Z);
}

void TestPhysicsWorld::TestParameterizedConstructor() {
    PhysicsWorld obj(Vector3D(0, -20, 0));
    Vector3D g = obj.GetGravity();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, g.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -20.0f, g.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, g.Z);
}

// ========== Method Tests ==========

void TestPhysicsWorld::TestAddBody() {
    PhysicsWorld obj;
    RigidBody body;
    obj.AddBody(&body);
    TEST_ASSERT_EQUAL(1, obj.GetBodyCount());
}

void TestPhysicsWorld::TestRemoveBody() {
    PhysicsWorld obj;
    RigidBody body;
    obj.AddBody(&body);
    TEST_ASSERT_EQUAL(1, obj.GetBodyCount());
    obj.RemoveBody(&body);
    TEST_ASSERT_EQUAL(0, obj.GetBodyCount());
}

void TestPhysicsWorld::TestRemoveAllBodies() {
    PhysicsWorld obj;
    RigidBody b1, b2, b3;
    obj.AddBody(&b1);
    obj.AddBody(&b2);
    obj.AddBody(&b3);
    TEST_ASSERT_EQUAL(3, obj.GetBodyCount());
    obj.RemoveAllBodies();
    TEST_ASSERT_EQUAL(0, obj.GetBodyCount());
}

void TestPhysicsWorld::TestGetBodyCount() {
    PhysicsWorld obj;
    TEST_ASSERT_EQUAL(0, obj.GetBodyCount());
    RigidBody b1, b2;
    obj.AddBody(&b1);
    TEST_ASSERT_EQUAL(1, obj.GetBodyCount());
    obj.AddBody(&b2);
    TEST_ASSERT_EQUAL(2, obj.GetBodyCount());
}

void TestPhysicsWorld::TestGetGravity() {
    PhysicsWorld obj;
    Vector3D g = obj.GetGravity();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, g.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -9.81f, g.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, g.Z);
}

void TestPhysicsWorld::TestSetGravity() {
    PhysicsWorld obj;
    obj.SetGravity(Vector3D(0, -15.0f, 0));
    Vector3D g = obj.GetGravity();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, g.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -15.0f, g.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, g.Z);
}

void TestPhysicsWorld::TestSetFixedTimestep() {
    PhysicsWorld obj;
    obj.SetFixedTimestep(0.02f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.02f, obj.GetFixedTimestep());
}

void TestPhysicsWorld::TestSetMaxSubSteps() {
    PhysicsWorld obj;
    obj.SetMaxSubSteps(5);
    TEST_ASSERT_EQUAL(5, obj.GetMaxSubSteps());
}

void TestPhysicsWorld::TestClearCollisionCallbacks() {
    PhysicsWorld obj;
    obj.ClearCollisionCallbacks();
    TEST_ASSERT_TRUE(true);
}

void TestPhysicsWorld::TestStep() {
    PhysicsWorld obj;
    SphereCollider collider(Vector3D(0, 10, 0), 1.0f);
    RigidBody body(BodyType::Dynamic, 1.0f);
    body.SetCollider(&collider);
    obj.AddBody(&body);
    obj.Step(1.0f / 60.0f);
    TEST_ASSERT_TRUE(true);
}

// Manifold-pipeline integration: sphere falls and rests on a static sphere
// via the manifold-pipeline path (broadphase -> narrowphase -> SI solver -> sleep).
// dynamic sphere falls onto a static 2-triangle mesh ground
// (BVH + mesh narrowphase). Verifies the new MeshCollider/TriangleMeshShape
// path through PhysicsWorld::FixedStep end-to-end.
;
    data->indices = { 0,1,2, 0,2,3 };
    data->Build();

    MeshCollider meshCol(data);
    RigidBody ground(BodyType::Static, 0.0f);
    ground.SetCollider(&meshCol);
    ground.SetPose(BodyPose(Vector3D(0,0,0), Quaternion()));

    SphereCollider sphereCol(Vector3D(0,0,0), 1.0f);
    RigidBody dyn(BodyType::Dynamic, 1.0f);
    dyn.SetCollider(&sphereCol);
    dyn.SetPose(BodyPose(Vector3D(0, 4.0f, 0), Quaternion()));

    world.AddBody(&ground);
    world.AddBody(&dyn);

    for (int i = 0; i < 240; ++i) world.Step(1.0f / 120.0f);

    // Sphere should rest above the ground (y > radius - small penetration tolerance).
    TEST_ASSERT_TRUE(dyn.GetPose().position.Y > 0.9f);
    TEST_ASSERT_TRUE(std::abs(dyn.GetVelocity().Y) < 0.5f);
}

// dynamic capsule falls onto static 2-triangle mesh ground.
;
    data->indices = { 0,1,2, 0,2,3 };
    data->Build();

    MeshCollider meshCol(data);
    RigidBody ground(BodyType::Static, 0.0f);
    ground.SetCollider(&meshCol);
    ground.SetPose(BodyPose(Vector3D(0,0,0), Quaternion()));

    // radius=0.5, full height=2 (cylinder length=1 + 2 hemispheres of r=0.5)
    CapsuleCollider capCol(Vector3D(0,0,0), 0.5f, 2.0f);
    RigidBody dyn(BodyType::Dynamic, 1.0f);
    dyn.SetCollider(&capCol);
    dyn.SetPose(BodyPose(Vector3D(0, 4.0f, 0), Quaternion()));

    world.AddBody(&ground);
    world.AddBody(&dyn);

    for (int i = 0; i < 240; ++i) world.Step(1.0f / 120.0f);

    // Capsule upright should rest with bottom hemisphere on the ground:
    // center y ≈ half-height (1.0). Allow penetration tolerance.
    TEST_ASSERT_TRUE(dyn.GetPose().position.Y > 0.9f);
    TEST_ASSERT_TRUE(std::abs(dyn.GetVelocity().Y) < 0.5f);
}

// dynamic box falls onto static 2-triangle mesh ground.
;
    data->indices = { 0,1,2, 0,2,3 };
    data->Build();

    MeshCollider meshCol(data);
    RigidBody ground(BodyType::Static, 0.0f);
    ground.SetCollider(&meshCol);
    ground.SetPose(BodyPose(Vector3D(0,0,0), Quaternion()));

    // 1x1x1 box (full size). Half-extent = 0.5.
    BoxCollider boxCol(Vector3D(0,0,0), Vector3D(1,1,1));
    RigidBody dyn(BodyType::Dynamic, 1.0f);
    dyn.SetCollider(&boxCol);
    dyn.SetPose(BodyPose(Vector3D(0, 4.0f, 0), Quaternion()));

    world.AddBody(&ground);
    world.AddBody(&dyn);

    for (int i = 0; i < 240; ++i) world.Step(1.0f / 120.0f);

    // Box should rest on ground: center y ≈ 0.5. Allow penetration tolerance.
    TEST_ASSERT_TRUE(dyn.GetPose().position.Y > 0.4f);
    TEST_ASSERT_TRUE(std::abs(dyn.GetVelocity().Y) < 0.5f);
}

// ========== Heightfield Integration ==========

namespace {
std::shared_ptr<HeightfieldData> MakeFlat6x6() {
    auto data = std::make_shared<HeightfieldData>();
    data->widthCells = 6;
    data->depthCells = 6;
    data->cellSize   = 1.0f;
    data->heights.assign(7 * 7, 0.0f);
    data->Build();
    return data;
}
}

}
    data->Build();

    HeightfieldCollider hfCol(data);
    RigidBody ground(BodyType::Static, 0.0f);
    ground.SetCollider(&hfCol);
    ground.SetPose(BodyPose(Vector3D(0,0,0), Quaternion()));

    SphereCollider sphereCol(Vector3D(0,0,0), 0.5f);
    RigidBody dyn(BodyType::Dynamic, 1.0f);
    dyn.SetCollider(&sphereCol);
    dyn.SetPose(BodyPose(Vector3D(0, 3.0f, 0), Quaternion()));

    world.AddBody(&ground);
    world.AddBody(&dyn);
    for (int i = 0; i < 240; ++i) world.Step(1.0f / 120.0f);

    // After settling on the ramp the sphere center sits above the local
    // surface (flat-rest height at x=0 would be 0.5; tilt drops that a bit).
    TEST_ASSERT_TRUE(dyn.GetPose().position.Y > -0.1f);
    // And it must have drifted downhill (-X). The slope component of gravity
    // dominates default friction, so this is conservative.
    TEST_ASSERT_TRUE(dyn.GetPose().position.X < -0.05f);
}

// ========== Mesh raycast ==========

;
    data->indices = { 0,1,2, 0,2,3 };
    data->Build();
    MeshCollider meshCol(data);
    meshCol.SetPosition(Vector3D(0,0,0));

    Ray ray(Vector3D(0, 5, 0), Vector3D(0, -1, 0));
    RaycastHit hit;
    bool ok = meshCol.Raycast(ray, hit, 100.0f);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 5.0f, hit.distance);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 1.0f, hit.normal.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, hit.point.Y);
}

;
    data->indices = { 0,1,2, 0,2,3 };
    data->Build();
    MeshCollider meshCol(data);

    // Ray pointing away from the mesh.
    Ray ray(Vector3D(0, 5, 0), Vector3D(0, 1, 0));
    RaycastHit hit;
    TEST_ASSERT_FALSE(meshCol.Raycast(ray, hit, 100.0f));

    // Ray well outside lateral extent.
    Ray ray2(Vector3D(10, 5, 10), Vector3D(0, -1, 0));
    TEST_ASSERT_FALSE(meshCol.Raycast(ray2, hit, 100.0f));
}

// ========== Edge Cases ==========

void TestPhysicsWorld::TestEdgeCases() {
    PhysicsWorld obj;
    // Remove nullptr should not crash
    obj.RemoveBody(nullptr);
    TEST_ASSERT_EQUAL(0, obj.GetBodyCount());

    // RemoveAllBodies on empty world
    obj.RemoveAllBodies();
    TEST_ASSERT_EQUAL(0, obj.GetBodyCount());
}

// ========== bullet flag plumbing ==========

// ========== Phase 8c: speculative-contact narrowphase ==========

);

    BoxCollider wallCol(Vector3D(0,0,0), Vector3D(0.5f, 5.0f, 5.0f));
    RigidBody wall(BodyType::Static, 0.0f);
    wall.SetCollider(&wallCol);
    wall.SetPose(BodyPose(Vector3D(0, 0, 0), Quaternion()));

    SphereCollider sphereCol(Vector3D(0,0,0), 0.5f);
    RigidBody bullet(BodyType::Dynamic, 1.0f);
    bullet.SetCollider(&sphereCol);
    bullet.SetPose(BodyPose(Vector3D(-2.5f, 0, 0), Quaternion()));
    bullet.SetVelocity(Vector3D(300, 0, 0));
    bullet.SetBullet(true);

    world.AddBody(&wall);
    world.AddBody(&bullet);
    // One step: speculative emit, but no real overlap.
    world.Step(1.0f / 120.0f);
    TEST_ASSERT_TRUE(world.GetLastStepSpeculativeManifolds() >= 1);
    TEST_ASSERT_EQUAL(0, enterCount);
}

;

    const float yNon    = runScenario(false);
    const float yBullet = runScenario(true);
    // Both should rest near y = 1.5 (sum of radii, accounting for slop). The
    // delta between bullet and non-bullet should be small - bullet must not
    // hover meaningfully higher than non-bullet because of premature
    // speculative-impulse arrest.
    TEST_ASSERT_TRUE(std::fabs(yBullet - yNon) < 0.1f);
}

// ========== Test Runner ==========

void TestPhysicsWorld::TestComputeDiagnostics() {
    // TODO: Implement test for ComputeDiagnostics()
    PhysicsWorld obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestPhysicsWorld::TestGetDebugContactCount() {
    // TODO: Implement test for GetDebugContactCount()
    PhysicsWorld obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestPhysicsWorld::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestAddBody);
    RUN_TEST(TestRemoveBody);
    RUN_TEST(TestRemoveAllBodies);
    RUN_TEST(TestGetBodyCount);
    RUN_TEST(TestGetGravity);
    RUN_TEST(TestSetGravity);
    RUN_TEST(TestSetFixedTimestep);
    RUN_TEST(TestSetMaxSubSteps);
    RUN_TEST(TestClearCollisionCallbacks);
    RUN_TEST(TestStep);

    RUN_TEST(TestEdgeCases);

    RUN_TEST(TestComputeDiagnostics);
    RUN_TEST(TestGetDebugContactCount);
}
