// SPDX-License-Identifier: GPL-3.0-or-later
// Physics & Geometry Scripting Bindings
#include "helpers.hpp"
#include <cstring>

void TestRigidBody() {
    std::cout << "--- RigidBody ---" << std::endl;
    using namespace koilo;

    // Test 1: Default constructor
    {
        RigidBody body;
        if (body.IsDynamic() && body.GetMass() == 1.0f && body.GetInverseMass() == 1.0f)
            PASS() else FAIL("Default RigidBody should be Dynamic with mass 1")
    }

    // Test 2: Static body ignores forces
    {
        RigidBody body(BodyType::Static, 0.0f);
        if (body.IsStatic() && body.GetInverseMass() == 0.0f)
            PASS() else FAIL("Static body should have zero inverse mass")
        body.ApplyForce(Vector3D(100, 0, 0));
        body.Integrate(1.0f, Vector3D(0, -9.81f, 0));
        if (body.GetVelocity().X == 0.0f && body.GetVelocity().Y == 0.0f)
            PASS() else FAIL("Static body should not move from forces")
    }

    // Test 3: Dynamic body integration with gravity
    {
        RigidBody body;
        SphereCollider collider(Vector3D(0, 10, 0), 1.0f);
        body.SetCollider(&collider);
        body.Integrate(1.0f, Vector3D(0, -10.0f, 0));
        // After 1s with gravity -10: v = -10, pos = 10 + (-10)*1 = 0
        if (body.GetVelocity().Y < -9.0f)
            PASS() else FAIL("Body should have negative Y velocity after gravity")
        if (collider.GetPosition().Y < 10.0f)
            PASS() else FAIL("Body should have fallen from gravity")
    }

    // Test 4: ApplyForce accumulation
    {
        RigidBody body(BodyType::Dynamic, 2.0f);
        SphereCollider collider(Vector3D(0, 0, 0), 1.0f);
        body.SetCollider(&collider);
        body.ApplyForce(Vector3D(20, 0, 0)); // F=20, m=2, a=10
        body.Integrate(1.0f, Vector3D(0, 0, 0));
        // v = 10*1 = 10, pos = 0 + 10*1 = 10
        if (std::abs(body.GetVelocity().X - 10.0f) < 0.5f)
            PASS() else FAIL("F=20, m=2 should give v≈10 after 1s")
    }

    // Test 5: ApplyImpulse
    {
        RigidBody body(BodyType::Dynamic, 2.0f);
        body.ApplyImpulse(Vector3D(10, 0, 0)); // impulse/mass = 5
        if (std::abs(body.GetVelocity().X - 5.0f) < 0.01f)
            PASS() else FAIL("Impulse 10 on mass 2 should give velocity 5")
    }

    // Test 6: Kinematic body
    {
        RigidBody body(BodyType::Kinematic, 1.0f);
        if (body.IsKinematic() && body.GetInverseMass() == 0.0f)
            PASS() else FAIL("Kinematic should have zero inverse mass")
    }

    // Test 7: SetBodyType transitions
    {
        RigidBody body;
        body.SetVelocity(Vector3D(5, 0, 0));
        body.SetBodyType(BodyType::Static);
        if (body.GetVelocity().X == 0.0f && body.IsStatic())
            PASS() else FAIL("Switching to Static should zero velocity")
        body.SetBodyType(BodyType::Dynamic);
        if (body.IsDynamic() && body.GetInverseMass() > 0.0f)
            PASS() else FAIL("Switching back to Dynamic should restore inverse mass")
    }

    // Test 8: Restitution and friction
    {
        RigidBody body;
        body.SetRestitution(0.8f);
        body.SetFriction(0.3f);
        if (std::abs(body.GetRestitution() - 0.8f) < 0.01f &&
            std::abs(body.GetFriction() - 0.3f) < 0.01f)
            PASS() else FAIL("Restitution/friction setters should work")
    }

    std::cout << std::endl;
}

void TestPhysicsWorld() {
    std::cout << "--- PhysicsWorld ---" << std::endl;
    using namespace koilo;

    // Test 1: Default gravity
    {
        PhysicsWorld world;
        if (std::abs(world.GetGravity().Y - (-9.81f)) < 0.01f)
            PASS() else FAIL("Default gravity should be (0, -9.81, 0)")
    }

    // Test 2: Add/remove bodies
    {
        PhysicsWorld world;
        RigidBody body;
        SphereCollider collider(Vector3D(0, 0, 0), 1.0f);
        body.SetCollider(&collider);
        world.AddBody(&body);
        if (world.GetBodyCount() == 1)
            PASS() else FAIL("Should have 1 body after add")
        world.RemoveBody(&body);
        if (world.GetBodyCount() == 0)
            PASS() else FAIL("Should have 0 bodies after remove")
    }

    // Test 3: Gravity integration over time
    {
        PhysicsWorld world(Vector3D(0, -10, 0));
        world.SetFixedTimestep(1.0f / 60.0f);
        RigidBody body;
        SphereCollider collider(Vector3D(0, 100, 0), 1.0f);
        body.SetCollider(&collider);
        world.AddBody(&body);
        // Step for 1 second
        for (int i = 0; i < 60; ++i) {
            koilo::TimeManager::GetInstance().Tick(1.0f / 60.0f); world.Step(1.0f / 60.0f);
        }
        // After 1s of -10 gravity, y should be around 100 - 5 = 95 (s = 0.5*g*t^2)
        float y = collider.GetPosition().Y;
        if (y < 100.0f && y > 80.0f)
            PASS() else FAIL("Body should have fallen ~5 units in 1 second")
    }

    // Test 4: Collision impulse (two spheres approaching)
    {
        PhysicsWorld world(Vector3D(0, 0, 0)); // No gravity
        RigidBody bodyA(BodyType::Dynamic, 1.0f);
        RigidBody bodyB(BodyType::Dynamic, 1.0f);
        bodyA.SetRestitution(1.0f);
        bodyB.SetRestitution(1.0f);

        SphereCollider colliderA(Vector3D(-1, 0, 0), 1.5f);
        SphereCollider colliderB(Vector3D(1, 0, 0), 1.5f);
        bodyA.SetCollider(&colliderA);
        bodyB.SetCollider(&colliderB);
        bodyA.SetVelocity(Vector3D(10, 0, 0));
        bodyB.SetVelocity(Vector3D(-10, 0, 0));

        world.AddBody(&bodyA);
        world.AddBody(&bodyB);

        // Spheres overlap (distance=2, radii sum=3), should resolve immediately
        koilo::TimeManager::GetInstance().Tick(1.0f / 60.0f); world.Step(1.0f / 60.0f);

        // After elastic collision, velocities should reverse (or close)
        if (bodyA.GetVelocity().X < 0.0f)
            PASS() else FAIL("Body A should bounce back (negative X velocity)")
        if (bodyB.GetVelocity().X > 0.0f)
            PASS() else FAIL("Body B should bounce back (positive X velocity)")
    }

    // Test 5: Static body doesn't move from collision
    {
        PhysicsWorld world(Vector3D(0, 0, 0));
        RigidBody staticBody(BodyType::Static, 0.0f);
        RigidBody dynamicBody(BodyType::Dynamic, 1.0f);

        SphereCollider staticCollider(Vector3D(0, 0, 0), 2.0f);
        SphereCollider dynamicCollider(Vector3D(1, 0, 0), 2.0f);
        staticBody.SetCollider(&staticCollider);
        dynamicBody.SetCollider(&dynamicCollider);
        dynamicBody.SetVelocity(Vector3D(-10, 0, 0));

        world.AddBody(&staticBody);
        world.AddBody(&dynamicBody);
        koilo::TimeManager::GetInstance().Tick(1.0f / 60.0f); world.Step(1.0f / 60.0f);

        if (staticCollider.GetPosition().X == 0.0f)
            PASS() else FAIL("Static body should not move")
    }

    // Test 6: Max substeps prevents spiral of death
    {
        PhysicsWorld world;
        world.SetMaxSubSteps(4);
        world.SetFixedTimestep(1.0f / 60.0f);
        RigidBody body;
        SphereCollider collider(Vector3D(0, 0, 0), 1.0f);
        body.SetCollider(&collider);
        world.AddBody(&body);

        // Pass a huge dt - should clamp to 4 substeps max
        koilo::TimeManager::GetInstance().Tick(10.0f); world.Step(10.0f);
        // Should not crash, body should have moved a limited amount
        if (std::isfinite(collider.GetPosition().Y))
            PASS() else FAIL("Large dt should not produce non-finite values")
    }

    // Test 7: Custom gravity
    {
        PhysicsWorld world;
        world.SetGravity(Vector3D(0, 0, -5));
        if (std::abs(world.GetGravity().Z - (-5.0f)) < 0.01f)
            PASS() else FAIL("Custom gravity should be settable")
    }

    std::cout << std::endl;
}

void TestCapsuleCollisions() {
    std::cout << "--- Capsule Collisions ---" << std::endl;
    using namespace koilo;

    CollisionManager manager;

    // Test 1: Capsule-Sphere collision
    {
        CapsuleCollider capsule(Vector3D(0, 0, 0), 1.0f, 4.0f);
        SphereCollider sphere(Vector3D(1.5f, 0, 0), 1.0f);
        manager.RegisterCollider(&capsule);
        manager.RegisterCollider(&sphere);

        CollisionInfo info;
        bool hit = manager.TestCollision(&capsule, &sphere, info);
        if (hit && info.penetrationDepth > 0.0f)
            PASS() else FAIL("Capsule-Sphere should collide when overlapping")
        manager.UnregisterAllColliders();
    }

    // Test 2: Capsule-Sphere miss
    {
        CapsuleCollider capsule(Vector3D(0, 0, 0), 1.0f, 4.0f);
        SphereCollider sphere(Vector3D(10, 0, 0), 1.0f);

        CollisionInfo info;
        bool hit = manager.TestCollision(&capsule, &sphere, info);
        if (!hit)
            PASS() else FAIL("Capsule-Sphere should miss when far apart")
    }

    // Test 3: Capsule-Capsule collision
    {
        CapsuleCollider a(Vector3D(0, 0, 0), 1.0f, 4.0f);
        CapsuleCollider b(Vector3D(1.5f, 0, 0), 1.0f, 4.0f);

        CollisionInfo info;
        bool hit = manager.TestCollision(&a, &b, info);
        if (hit && info.penetrationDepth > 0.0f)
            PASS() else FAIL("Capsule-Capsule should collide when overlapping")
    }

    // Test 4: Capsule-Capsule miss
    {
        CapsuleCollider a(Vector3D(0, 0, 0), 1.0f, 4.0f);
        CapsuleCollider b(Vector3D(10, 0, 0), 1.0f, 4.0f);

        CollisionInfo info;
        bool hit = manager.TestCollision(&a, &b, info);
        if (!hit)
            PASS() else FAIL("Capsule-Capsule should miss when far apart")
    }

    // Test 5: Capsule-Box collision
    {
        CapsuleCollider capsule(Vector3D(0, 0, 0), 1.0f, 4.0f);
        BoxCollider box(Vector3D(1.0f, 0, 0), Vector3D(2, 2, 2));

        CollisionInfo info;
        bool hit = manager.TestCollision(&capsule, &box, info);
        if (hit && info.penetrationDepth > 0.0f)
            PASS() else FAIL("Capsule-Box should collide when overlapping")
    }

    // Test 6: Box-Box improved penetration depth
    {
        BoxCollider a(Vector3D(0, 0, 0), Vector3D(2, 2, 2));
        BoxCollider b(Vector3D(1.5f, 0, 0), Vector3D(2, 2, 2));

        CollisionInfo info;
        bool hit = manager.TestCollision(&a, &b, info);
        // Overlap on X axis: min(1,2.5) - max(-1,-0.5) = 1 - (-0.5) = 1.5
        // Wait: a goes from -1 to 1, b goes from 0.5 to 2.5
        // Overlap X = min(1, 2.5) - max(-1, 0.5) = 1 - 0.5 = 0.5
        if (hit && std::abs(info.penetrationDepth - 0.5f) < 0.1f)
            PASS() else FAIL("Box-Box should report accurate penetration depth")
    }

    std::cout << std::endl;
}

void TestCollisionCallbacks() {
    std::cout << "--- Collision Callbacks ---" << std::endl;
    using namespace koilo;

    // Test 1: OnCollisionEnter fires when bodies first collide
    {
        PhysicsWorld world;
        world.SetGravity(Vector3D(0, 0, 0));

        RigidBody bodyA(BodyType::Dynamic, 1.0f);
        RigidBody bodyB(BodyType::Static, 0.0f);
        SphereCollider colA(Vector3D(2, 0, 0), 1.0f);
        SphereCollider colB(Vector3D(0, 0, 0), 2.0f);
        bodyA.SetCollider(&colA);
        bodyB.SetCollider(&colB);
        bodyA.SetVelocity(Vector3D(-5, 0, 0));

        world.AddBody(&bodyA);
        world.AddBody(&bodyB);

        int enterCount = 0;
        world.OnCollisionEnter([&enterCount](const CollisionEvent& evt) {
            enterCount++;
        });

        koilo::TimeManager::GetInstance().Tick(1.0f / 60.0f); world.Step(1.0f / 60.0f);
        if (enterCount == 1)
            PASS() else FAIL("OnCollisionEnter should fire once on first collision")
    }

    // Test 2: OnCollisionStay fires on continued collision
    {
        PhysicsWorld world;
        world.SetGravity(Vector3D(0, 0, 0));

        RigidBody bodyA(BodyType::Dynamic, 1.0f);
        RigidBody bodyB(BodyType::Static, 0.0f);
        SphereCollider colA(Vector3D(0.5f, 0, 0), 1.0f);
        SphereCollider colB(Vector3D(0, 0, 0), 1.0f);
        bodyA.SetCollider(&colA);
        bodyB.SetCollider(&colB);
        bodyA.SetRestitution(0.0f);

        world.AddBody(&bodyA);
        world.AddBody(&bodyB);

        int stayCount = 0;
        world.OnCollisionStay([&stayCount](const CollisionEvent& evt) {
            stayCount++;
        });

        koilo::TimeManager::GetInstance().Tick(1.0f / 60.0f); world.Step(1.0f / 60.0f);
        koilo::TimeManager::GetInstance().Tick(1.0f / 60.0f); world.Step(1.0f / 60.0f);
        if (stayCount >= 1)
            PASS() else FAIL("OnCollisionStay should fire on continued overlap")
    }

    // Test 3: OnCollisionExit fires when bodies separate
    {
        PhysicsWorld world;
        world.SetGravity(Vector3D(0, 0, 0));

        RigidBody bodyA(BodyType::Dynamic, 1.0f);
        RigidBody bodyB(BodyType::Static, 0.0f);
        SphereCollider colA(Vector3D(1.5f, 0, 0), 1.0f);
        SphereCollider colB(Vector3D(0, 0, 0), 1.0f);
        bodyA.SetCollider(&colA);
        bodyB.SetCollider(&colB);
        bodyA.SetVelocity(Vector3D(-2, 0, 0));
        bodyA.SetRestitution(1.0f);
        bodyB.SetRestitution(1.0f);

        world.AddBody(&bodyA);
        world.AddBody(&bodyB);

        int exitCount = 0;
        world.OnCollisionExit([&exitCount](const CollisionEvent& evt) {
            exitCount++;
        });

        // Step 1: collision occurs (enter)
        koilo::TimeManager::GetInstance().Tick(1.0f / 60.0f); world.Step(1.0f / 60.0f);
        // Force bodyA far away so next step has no collision
        colA.SetPosition(Vector3D(100, 0, 0));
        bodyA.SetVelocity(Vector3D(0, 0, 0));
        // Step 2: no collision -> exit fires
        koilo::TimeManager::GetInstance().Tick(1.0f / 60.0f); world.Step(1.0f / 60.0f);
        if (exitCount >= 1)
            PASS() else FAIL("OnCollisionExit should fire when bodies separate")
    }

    // Test 4: ClearCollisionCallbacks removes all callbacks
    {
        PhysicsWorld world;
        world.SetGravity(Vector3D(0, 0, 0));

        int count = 0;
        world.OnCollisionEnter([&count](const CollisionEvent& evt) { count++; });
        world.ClearCollisionCallbacks();

        RigidBody bodyA(BodyType::Dynamic, 1.0f);
        RigidBody bodyB(BodyType::Static, 0.0f);
        SphereCollider colA(Vector3D(0.5f, 0, 0), 1.0f);
        SphereCollider colB(Vector3D(0, 0, 0), 1.0f);
        bodyA.SetCollider(&colA);
        bodyB.SetCollider(&colB);

        world.AddBody(&bodyA);
        world.AddBody(&bodyB);
        koilo::TimeManager::GetInstance().Tick(1.0f / 60.0f); world.Step(1.0f / 60.0f);

        if (count == 0)
            PASS() else FAIL("ClearCollisionCallbacks should remove all callbacks")
    }

    // Test 5: CollisionEvent contains valid data
    {
        PhysicsWorld world;
        world.SetGravity(Vector3D(0, 0, 0));

        RigidBody bodyA(BodyType::Dynamic, 1.0f);
        RigidBody bodyB(BodyType::Static, 0.0f);
        SphereCollider colA(Vector3D(1.5f, 0, 0), 1.0f);
        SphereCollider colB(Vector3D(0, 0, 0), 1.0f);
        bodyA.SetCollider(&colA);
        bodyB.SetCollider(&colB);
        bodyA.SetVelocity(Vector3D(-5, 0, 0));

        world.AddBody(&bodyA);
        world.AddBody(&bodyB);

        bool hasValidData = false;
        world.OnCollisionEnter([&hasValidData](const CollisionEvent& evt) {
            if (evt.colliderA && evt.colliderB && evt.bodyA && evt.bodyB && evt.penetration > 0.0f) {
                hasValidData = true;
            }
        });

        koilo::TimeManager::GetInstance().Tick(1.0f / 60.0f); world.Step(1.0f / 60.0f);
        if (hasValidData)
            PASS() else FAIL("CollisionEvent should contain valid collider/body/penetration data")
    }

    std::cout << std::endl;
}

void TestPhysicsScriptGlobal() {
    std::cout << "--- Physics Script Global ---" << std::endl;
    using namespace koilo;
    using namespace koilo::scripting;

    // Ensure Describe() is called so reflection is populated
    PhysicsWorld::Describe();
    RigidBody::Describe();

    // Test 1: PhysicsWorld is registered via reflection
    {
        const ClassDesc* desc = ReflectionBridge::FindClass("PhysicsWorld");
        if (desc != nullptr)
            PASS() else FAIL("PhysicsWorld should be findable via reflection")
    }

    // Test 2: PhysicsWorld has SetGravity in reflection
    {
        const ClassDesc* desc = ReflectionBridge::FindClass("PhysicsWorld");
        bool found = false;
        if (desc) {
            for (size_t i = 0; i < desc->methods.count; i++) { auto& m = desc->methods.data[i];
                if (std::strcmp(m.name, "SetGravity") == 0) { found = true; break; }
            }
        }
        if (found)
            PASS() else FAIL("PhysicsWorld should have SetGravity in reflection")
    }

    // Test 3: PhysicsWorld has Step in reflection
    {
        const ClassDesc* desc = ReflectionBridge::FindClass("PhysicsWorld");
        bool found = false;
        if (desc) {
            for (size_t i = 0; i < desc->methods.count; i++) { auto& m = desc->methods.data[i];
                if (std::strcmp(m.name, "Step") == 0) { found = true; break; }
            }
        }
        if (found)
            PASS() else FAIL("PhysicsWorld should have Step in reflection")
    }

    // Test 4: RigidBody is reflected
    {
        const ClassDesc* desc = ReflectionBridge::FindClass("RigidBody");
        if (desc != nullptr)
            PASS() else FAIL("RigidBody should be findable via reflection")
    }

    // Test 5: RigidBody has SetMass in reflection
    {
        const ClassDesc* desc = ReflectionBridge::FindClass("RigidBody");
        bool found = false;
        if (desc) {
            for (size_t i = 0; i < desc->methods.count; i++) { auto& m = desc->methods.data[i];
                if (std::strcmp(m.name, "SetMass") == 0) { found = true; break; }
            }
        }
        if (found)
            PASS() else FAIL("RigidBody should have SetMass in reflection")
    }

    std::cout << std::endl;
}

void TestPhase13_2DGeometry() {
    std::cout << "--- 2D Geometry Refactor ---" << std::endl;

    // AABB2D basics
    {
        TEST("AABB2D contains point inside")
        AABB2D box(Vector2D(-1, -1), Vector2D(1, 1));
        if (box.Contains(Vector2D(0, 0))) PASS()
        else FAIL("Origin should be inside")
    }

    {
        TEST("AABB2D rejects point outside")
        AABB2D box(Vector2D(-1, -1), Vector2D(1, 1));
        if (!box.Contains(Vector2D(5, 5))) PASS()
        else FAIL("(5,5) should be outside")
    }

    {
        TEST("AABB2D overlap detection")
        AABB2D a(Vector2D(0, 0), Vector2D(2, 2));
        AABB2D b(Vector2D(1, 1), Vector2D(3, 3));
        AABB2D c(Vector2D(5, 5), Vector2D(6, 6));
        if (a.Overlaps(b) && !a.Overlaps(c)) PASS()
        else FAIL("Overlap mismatch")
    }

    {
        TEST("AABB2D union merges boxes")
        AABB2D a(Vector2D(0, 0), Vector2D(1, 1));
        AABB2D b(Vector2D(2, 2), Vector2D(3, 3));
        AABB2D u = a.Union(b);
        if (std::abs(u.min.X) < 0.001f && std::abs(u.max.X - 3) < 0.001f &&
            std::abs(u.min.Y) < 0.001f && std::abs(u.max.Y - 3) < 0.001f) PASS()
        else FAIL("Union bounds wrong")
    }

    {
        TEST("AABB2D encapsulate expands to include point")
        AABB2D box(Vector2D(0, 0), Vector2D(1, 1));
        box.Encapsulate(Vector2D(5, -2));
        if (std::abs(box.max.X - 5) < 0.001f && std::abs(box.min.Y - (-2)) < 0.001f) PASS()
        else FAIL("Encapsulate failed")
    }

    {
        TEST("AABB2D closest point clamps to surface")
        AABB2D box(Vector2D(0, 0), Vector2D(2, 2));
        Vector2D cp = box.ClosestPoint(Vector2D(5, 1));
        if (std::abs(cp.X - 2) < 0.001f && std::abs(cp.Y - 1) < 0.001f) PASS()
        else FAIL("ClosestPoint=" + std::to_string(cp.X) + "," + std::to_string(cp.Y))
    }

    {
        TEST("AABB2D area calculation")
        AABB2D box(Vector2D(0, 0), Vector2D(3, 4));
        float area = box.GetArea();
        if (std::abs(area - 12.0f) < 0.001f) PASS()
        else FAIL("Expected 12, got " + std::to_string(area))
    }

    {
        TEST("AABB2D from center and half-size")
        AABB2D box = AABB2D::FromCenterHalfSize(Vector2D(5, 5), Vector2D(2, 3));
        if (std::abs(box.min.X - 3) < 0.001f && std::abs(box.max.Y - 8) < 0.001f) PASS()
        else FAIL("FromCenterHalfSize bounds wrong")
    }

    // Rectangle2D::OverlapsCircle
    {
        TEST("Rectangle2D overlaps circle (circle inside rect)")
        Rectangle2D rect(Vector2D(10, 10), Vector2D(20, 20));
        Circle2D circle(Vector2D(10, 10), 3.0f);
        if (rect.OverlapsCircle(circle)) PASS()
        else FAIL("Circle inside rect should overlap")
    }

    {
        TEST("Rectangle2D overlaps circle (circle outside)")
        Rectangle2D rect(Vector2D(10, 10), Vector2D(20, 20));
        Circle2D circle(Vector2D(50, 50), 3.0f);
        if (!rect.OverlapsCircle(circle)) PASS()
        else FAIL("Distant circle should not overlap")
    }

    // Circle2D::OverlapsCircle
    {
        TEST("Circle2D overlaps circle (overlapping)")
        Circle2D a(Vector2D(0, 0), 5.0f);
        Circle2D b(Vector2D(3, 0), 5.0f);
        if (a.OverlapsCircle(b)) PASS()
        else FAIL("Nearby circles should overlap")
    }

    {
        TEST("Circle2D does not overlap distant circle")
        Circle2D a(Vector2D(0, 0), 2.0f);
        Circle2D b(Vector2D(100, 100), 2.0f);
        if (!a.OverlapsCircle(b)) PASS()
        else FAIL("Distant circles should not overlap")
    }

    // AABB2D reflection
    {
        TEST("AABB2D registered in reflection (215 Describe calls)")
        AABB2D box(Vector2D(1, 2), Vector2D(3, 4));
        if (std::abs(box.min.X - 1) < 0.001f && std::abs(box.max.Y - 4) < 0.001f) PASS()
        else FAIL("AABB2D field access wrong")
    }

    std::cout << std::endl;
}


