// SPDX-License-Identifier: GPL-3.0-or-later
#include "testislandsandsleep.hpp"

#include <koilo/systems/physics/solver/islandbuilder.hpp>
#include <koilo/systems/physics/solver/sleepmanager.hpp>
#include <koilo/systems/physics/solver/sequentialimpulsesolver.hpp>
#include <koilo/systems/physics/contactmanifold.hpp>
#include <koilo/systems/physics/contactcache.hpp>
#include <koilo/systems/physics/colliderproxy.hpp>
#include <koilo/systems/physics/rigidbody.hpp>
#include <utils/testhelpers.hpp>

#include <vector>

using namespace koilo;

namespace {

RigidBody MakeDynamic(float mass, const Vector3D& pos) {
    RigidBody rb(BodyType::Dynamic, mass);
    rb.SetPose(BodyPose(pos));
    rb.SetInertiaSphere(0.5f);
    rb.SetFriction(0.0f);
    rb.SetRestitution(0.0f);
    return rb;
}

RigidBody MakeStatic(const Vector3D& pos) {
    RigidBody rb(BodyType::Static, 0.0f);
    rb.SetPose(BodyPose(pos));
    return rb;
}

ColliderProxy MakeProxy(std::uint32_t bodyId, std::uint32_t proxyId) {
    ColliderProxy p;
    p.bodyId = bodyId;
    p.proxyId = proxyId;
    return p;
}

ContactManifold MakeManifold(ColliderProxy* a, ColliderProxy* b) {
    ContactManifold m;
    m.a = a; m.b = b;
    m.count = 1;
    m.contacts[0].point = Vector3D(0,0,0);
    m.contacts[0].normal = Vector3D(0,1,0);
    m.contacts[0].depth = 0.0f;
    m.contacts[0].featureId = 1;
    return m;
}

} // namespace

void TestIslandsAndSleep::TestUnionFindGroupsConnectedBodies() {
    // 3 dynamic bodies, A-B and B-C connected -> one island {A,B,C}
    RigidBody a = MakeDynamic(1.0f, Vector3D(0,0,0));
    RigidBody b = MakeDynamic(1.0f, Vector3D(1,0,0));
    RigidBody c = MakeDynamic(1.0f, Vector3D(2,0,0));
    ColliderProxy pa = MakeProxy(0,1), pb = MakeProxy(1,2), pc = MakeProxy(2,3);
    std::vector<ContactManifold> ms = { MakeManifold(&pa,&pb), MakeManifold(&pb,&pc) };
    std::vector<RigidBody*> bodies = { &a, &b, &c };

    auto islands = IslandBuilder::Build(ms, bodies, false);
    TEST_ASSERT_EQUAL_UINT32(1u, static_cast<std::uint32_t>(islands.size()));
    TEST_ASSERT_EQUAL_UINT32(0u, islands[0].canonicalId);
    TEST_ASSERT_EQUAL_UINT32(3u, static_cast<std::uint32_t>(islands[0].bodyIds.size()));
    TEST_ASSERT_EQUAL_UINT32(2u, static_cast<std::uint32_t>(islands[0].manifoldIdx.size()));
}

void TestIslandsAndSleep::TestStaticBodyDoesNotBridgeIslands() {
    // Two dynamic bodies each touching the same static plane -> 2 islands.
    RigidBody a = MakeDynamic(1.0f, Vector3D(0,0,0));
    RigidBody s = MakeStatic(Vector3D(0,-1,0));
    RigidBody b = MakeDynamic(1.0f, Vector3D(5,0,0));
    ColliderProxy pa = MakeProxy(0,1), ps = MakeProxy(1,2), pb = MakeProxy(2,3);
    std::vector<ContactManifold> ms = { MakeManifold(&pa,&ps), MakeManifold(&pb,&ps) };
    std::vector<RigidBody*> bodies = { &a, &s, &b };

    auto islands = IslandBuilder::Build(ms, bodies, false);
    TEST_ASSERT_EQUAL_UINT32(2u, static_cast<std::uint32_t>(islands.size()));
    // Canonical ids = 0 and 2 (static body is not a member).
    TEST_ASSERT_EQUAL_UINT32(0u, islands[0].canonicalId);
    TEST_ASSERT_EQUAL_UINT32(2u, islands[1].canonicalId);
}

void TestIslandsAndSleep::TestSleepingBodyAnchorsByDefault() {
    // Sleeping middle body. With treatSleepingAsBridge=false -> A and C in separate islands.
    RigidBody a = MakeDynamic(1.0f, Vector3D(0,0,0));
    RigidBody mid = MakeDynamic(1.0f, Vector3D(1,0,0));
    mid.Sleep();
    RigidBody c = MakeDynamic(1.0f, Vector3D(2,0,0));
    ColliderProxy pa = MakeProxy(0,1), pm = MakeProxy(1,2), pc = MakeProxy(2,3);
    std::vector<ContactManifold> ms = { MakeManifold(&pa,&pm), MakeManifold(&pm,&pc) };
    std::vector<RigidBody*> bodies = { &a, &mid, &c };

    auto islands = IslandBuilder::Build(ms, bodies, /*treatSleepingAsBridge=*/false);
    // A is its own island; mid is its own island (no awake bridge); C is its own.
    TEST_ASSERT_EQUAL_UINT32(3u, static_cast<std::uint32_t>(islands.size()));
}

void TestIslandsAndSleep::TestSleepingBodyBridgesWhenRequested() {
    RigidBody a = MakeDynamic(1.0f, Vector3D(0,0,0));
    RigidBody mid = MakeDynamic(1.0f, Vector3D(1,0,0));
    mid.Sleep();
    RigidBody c = MakeDynamic(1.0f, Vector3D(2,0,0));
    ColliderProxy pa = MakeProxy(0,1), pm = MakeProxy(1,2), pc = MakeProxy(2,3);
    std::vector<ContactManifold> ms = { MakeManifold(&pa,&pm), MakeManifold(&pm,&pc) };
    std::vector<RigidBody*> bodies = { &a, &mid, &c };

    auto islands = IslandBuilder::Build(ms, bodies, /*treatSleepingAsBridge=*/true);
    TEST_ASSERT_EQUAL_UINT32(1u, static_cast<std::uint32_t>(islands.size()));
    TEST_ASSERT_EQUAL_UINT32(3u, static_cast<std::uint32_t>(islands[0].bodyIds.size()));
}

void TestIslandsAndSleep::TestSleepManagerSleepsQuietIsland() {
    RigidBody a = MakeDynamic(1.0f, Vector3D(0,0,0));
    RigidBody b = MakeDynamic(1.0f, Vector3D(1,0,0));
    a.SetVelocityRaw(Vector3D(0,0,0));
    b.SetVelocityRaw(Vector3D(0,0,0));
    ColliderProxy pa = MakeProxy(0,1), pb = MakeProxy(1,2);
    std::vector<ContactManifold> ms = { MakeManifold(&pa,&pb) };
    std::vector<RigidBody*> bodies = { &a, &b };

    SleepConfig cfg; // defaults: 0.5s required
    auto islands = IslandBuilder::Build(ms, bodies, false);
    const float dt = 0.1f;
    // Need 5 steps of dt=0.1 to exceed 0.5s.
    for (int i = 0; i < 6; ++i) {
        SleepManager::AttemptSleep(islands, bodies, dt, cfg);
    }
    TEST_ASSERT_TRUE(a.IsSleeping());
    TEST_ASSERT_TRUE(b.IsSleeping());
    // Velocities zeroed.
    TEST_ASSERT_EQUAL_FLOAT(0.0f, a.GetVelocity().Y);
}

void TestIslandsAndSleep::TestSleepManagerWakesOnMotion() {
    // Sleeping body chain bumped by an awake moving body should wake all.
    RigidBody a = MakeDynamic(1.0f, Vector3D(0,0,0));
    RigidBody b = MakeDynamic(1.0f, Vector3D(1,0,0));
    RigidBody c = MakeDynamic(1.0f, Vector3D(2,0,0));
    b.Sleep();
    c.Sleep();
    a.SetVelocityRaw(Vector3D(5,0,0));      // 0.5*1*25 = 12.5 J >> wake threshold 0.10
    ColliderProxy pa = MakeProxy(0,1), pb = MakeProxy(1,2), pc = MakeProxy(2,3);
    std::vector<ContactManifold> ms = { MakeManifold(&pa,&pb), MakeManifold(&pb,&pc) };
    std::vector<RigidBody*> bodies = { &a, &b, &c };

    auto bridge = IslandBuilder::Build(ms, bodies, /*treatSleepingAsBridge=*/true);
    SleepConfig cfg;
    SleepManager::WakeIslandsWithMotion(bridge, bodies, cfg);

    TEST_ASSERT_FALSE(a.IsSleeping());
    TEST_ASSERT_FALSE(b.IsSleeping());
    TEST_ASSERT_FALSE(c.IsSleeping());
}

void TestIslandsAndSleep::TestAllowSleepFalseKeepsBodyAwake() {
    RigidBody a = MakeDynamic(1.0f, Vector3D(0,0,0));
    RigidBody b = MakeDynamic(1.0f, Vector3D(1,0,0));
    a.SetAllowSleep(false);
    ColliderProxy pa = MakeProxy(0,1), pb = MakeProxy(1,2);
    std::vector<ContactManifold> ms = { MakeManifold(&pa,&pb) };
    std::vector<RigidBody*> bodies = { &a, &b };

    SleepConfig cfg;
    auto islands = IslandBuilder::Build(ms, bodies, false);
    for (int i = 0; i < 20; ++i) SleepManager::AttemptSleep(islands, bodies, 0.1f, cfg);

    TEST_ASSERT_FALSE(a.IsSleeping());
    TEST_ASSERT_FALSE(b.IsSleeping()); // protected via island co-membership
}

void TestIslandsAndSleep::TestSetVelocityWakesBody() {
    RigidBody a = MakeDynamic(1.0f, Vector3D(0,0,0));
    a.Sleep();
    TEST_ASSERT_TRUE(a.IsSleeping());
    a.SetVelocity(Vector3D(1,0,0));
    TEST_ASSERT_FALSE(a.IsSleeping());

    // Raw setter does NOT wake.
    a.Sleep();
    a.SetVelocityRaw(Vector3D(2,0,0));
    TEST_ASSERT_TRUE(a.IsSleeping());
}

void TestIslandsAndSleep::TestSolverTreatsSleepingBodyAsStatic() {
    // Awake ball moving downward into a sleeping body. Sleeping body acts as
    // an immovable wall -> no velocity change for sleeping body; ball's vn ≈ 0.
    RigidBody ball = MakeDynamic(1.0f, Vector3D(0, 0.5f, 0));
    ball.SetVelocity(Vector3D(0, -2.0f, 0));
    RigidBody floor = MakeDynamic(1.0f, Vector3D(0, 0, 0));
    floor.Sleep();

    ColliderProxy pball = MakeProxy(0,1), pfloor = MakeProxy(1,2);
    ContactManifold m;
    m.a = &pball; m.b = &pfloor; m.count = 1;
    m.contacts[0].point = Vector3D(0,0,0);
    m.contacts[0].normal = Vector3D(0,1,0);
    m.contacts[0].depth = 0.0f;
    m.contacts[0].featureId = 1;

    std::vector<ContactManifold> ms = { m };
    std::vector<RigidBody*> bodies = { &ball, &floor };
    ContactCache cache;
    cache.Touch(ms[0]);

    SequentialImpulseSolver solver;
    SolverConfig cfg;
    solver.Solve(ms, bodies, cache, 1.0f/120.0f, cfg);

    TEST_ASSERT_FLOAT_WITHIN(0.05f, 0.0f, ball.GetVelocity().Y);
    // Sleeping body unaffected.
    TEST_ASSERT_TRUE(floor.IsSleeping());
    TEST_ASSERT_EQUAL_FLOAT(0.0f, floor.GetVelocity().Y);
}

void TestIslandsAndSleep::RunAllTests() {
    RUN_TEST(TestUnionFindGroupsConnectedBodies);
    RUN_TEST(TestStaticBodyDoesNotBridgeIslands);
    RUN_TEST(TestSleepingBodyAnchorsByDefault);
    RUN_TEST(TestSleepingBodyBridgesWhenRequested);
    RUN_TEST(TestSleepManagerSleepsQuietIsland);
    RUN_TEST(TestSleepManagerWakesOnMotion);
    RUN_TEST(TestAllowSleepFalseKeepsBodyAwake);
    RUN_TEST(TestSetVelocityWakesBody);
    RUN_TEST(TestSolverTreatsSleepingBodyAsStatic);
}
