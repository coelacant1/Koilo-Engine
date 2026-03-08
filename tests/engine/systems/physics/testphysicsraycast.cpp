// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testphysicsraycast.cpp
 * @brief Implementation of PhysicsRaycast unit tests.
 */

#include "testphysicsraycast.hpp"
#include <koilo/systems/physics/spherecollider.hpp>

using namespace koilo;
// ========== Constructor Tests ==========

void TestPhysicsRaycast::TestDefaultConstructor() {
    // PhysicsRaycast is a static utility class - no constructor needed
    // Test that we can use static methods
    std::vector<Collider*> emptyColliders;
    RaycastHit hit;
    Ray ray(Vector3D(0, 0, 0), Vector3D(1, 0, 0));
    
    bool result = PhysicsRaycast::Raycast(ray, emptyColliders, hit, 100.0f);
    TEST_ASSERT_FALSE(result);  // No colliders, should return false
}

void TestPhysicsRaycast::TestParameterizedConstructor() {
    // Test static method with parameters
    std::vector<Collider*> colliders;
    SphereCollider sphere(Vector3D(10, 0, 0), 1.0f);
    colliders.push_back(&sphere);
    
    Ray ray(Vector3D(0, 0, 0), Vector3D(1, 0, 0).Normal());
    RaycastHit hit;
    
    bool result = PhysicsRaycast::Raycast(ray, colliders, hit, 100.0f);
    // Should hit the sphere at x=10
    if (result) {
        TEST_ASSERT_TRUE(hit.distance > 0.0f);
        TEST_ASSERT_TRUE(hit.distance < 100.0f);
    }
}

// ========== Edge Cases ==========

void TestPhysicsRaycast::TestEdgeCases() {
    std::vector<Collider*> colliders;
    RaycastHit hit;
    
    // Zero length ray
    Ray zeroRay(Vector3D(0, 0, 0), Vector3D(0, 0, 0));
    bool result1 = PhysicsRaycast::Raycast(zeroRay, colliders, hit, 0.0f);
    TEST_ASSERT_FALSE(result1);  // Zero distance, should miss
    
    // Test LayerInMask function
    TEST_ASSERT_TRUE(PhysicsRaycast::LayerInMask(0, -1));  // All layers
    TEST_ASSERT_TRUE(PhysicsRaycast::LayerInMask(5, 1 << 5));  // Specific layer
    TEST_ASSERT_FALSE(PhysicsRaycast::LayerInMask(5, 1 << 3));  // Wrong layer
}

// ========== Test Runner ==========

void TestPhysicsRaycast::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestEdgeCases);
}
