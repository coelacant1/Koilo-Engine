// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testcollisionmanager.cpp
 * @brief Implementation of CollisionManager unit tests.
 */

#include "testcollisionmanager.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

void TestCollisionManager::TestDefaultConstructor() {
    CollisionManager manager;
    // Verify initial state - no colliders registered
    TEST_ASSERT_EQUAL_size_t(0, manager.GetColliderCount());
}

void TestCollisionManager::TestParameterizedConstructor() {
    CollisionManager manager;
    // CollisionManager only has default constructor
    TEST_ASSERT_EQUAL_size_t(0, manager.GetColliderCount());
}

// ========== Method Tests ==========

void TestCollisionManager::TestRegisterCollider() {
    CollisionManager manager;
    SphereCollider collider(Vector3D(0, 0, 0), 1.0f);
    
    // Register collider
    manager.RegisterCollider(&collider);
    TEST_ASSERT_EQUAL_size_t(1, manager.GetColliderCount());
}

void TestCollisionManager::TestUnregisterCollider() {
    CollisionManager manager;
    SphereCollider collider(Vector3D(0, 0, 0), 1.0f);
    
    manager.RegisterCollider(&collider);
    TEST_ASSERT_EQUAL_size_t(1, manager.GetColliderCount());
    
    manager.UnregisterCollider(&collider);
    TEST_ASSERT_EQUAL_size_t(0, manager.GetColliderCount());
}

void TestCollisionManager::TestUnregisterAllColliders() {
    CollisionManager manager;
    SphereCollider c1(Vector3D(0, 0, 0), 1.0f);
    SphereCollider c2(Vector3D(5, 0, 0), 1.0f);
    
    manager.RegisterCollider(&c1);
    manager.RegisterCollider(&c2);
    TEST_ASSERT_EQUAL_size_t(2, manager.GetColliderCount());
    
    manager.UnregisterAllColliders();
    TEST_ASSERT_EQUAL_size_t(0, manager.GetColliderCount());
}

void TestCollisionManager::TestSetLayerCollision() {
    CollisionManager manager;
    
    // Set layer 0 and 1 to collide
    manager.SetLayerCollision(0, 1, true);
    TEST_ASSERT_TRUE(manager.CanLayersCollide(0, 1));
    
    // Set them to not collide
    manager.SetLayerCollision(0, 1, false);
    TEST_ASSERT_FALSE(manager.CanLayersCollide(0, 1));
}

void TestCollisionManager::TestCanLayersCollide() {
    CollisionManager manager;
    manager.SetDefaultCollisionMatrix();
    
    // Default matrix - all layers collide
    TEST_ASSERT_TRUE(manager.CanLayersCollide(0, 0));
    TEST_ASSERT_TRUE(manager.CanLayersCollide(0, 1));
    TEST_ASSERT_TRUE(manager.CanLayersCollide(15, 20));
}

void TestCollisionManager::TestSetDefaultCollisionMatrix() {
    CollisionManager manager;
    
    // Disable some collisions
    manager.SetLayerCollision(0, 1, false);
    TEST_ASSERT_FALSE(manager.CanLayersCollide(0, 1));
    
    // Reset to default - all collide
    manager.SetDefaultCollisionMatrix();
    TEST_ASSERT_TRUE(manager.CanLayersCollide(0, 1));
}

void TestCollisionManager::TestUpdate() {
    CollisionManager manager;
    
    // Update with no colliders should not crash
    manager.Update();
    TEST_ASSERT_EQUAL_size_t(0, manager.GetColliderCount());
    
    // Update with colliders
    SphereCollider collider(Vector3D(0, 0, 0), 1.0f);
    manager.RegisterCollider(&collider);
    manager.Update();
    TEST_ASSERT_EQUAL_size_t(1, manager.GetColliderCount());
}

void TestCollisionManager::TestClearCallbacks() {
    CollisionManager manager;
    
    // Clear callbacks should not crash even if none registered
    manager.ClearCallbacks();
    TEST_ASSERT_EQUAL_size_t(0, manager.GetColliderCount());
}

// ========== Edge Cases ==========

void TestCollisionManager::TestEdgeCases() {
    CollisionManager manager;
    
    // Update with zero delta time
    manager.Update();
    TEST_ASSERT_EQUAL_size_t(0, manager.GetColliderCount());
    
    // Layer collision at boundaries (0-31)
    manager.SetLayerCollision(0, 31, true);
    TEST_ASSERT_TRUE(manager.CanLayersCollide(0, 31));
    
    // Symmetric layer collision (order shouldn't matter)
    manager.SetLayerCollision(5, 10, false);
    TEST_ASSERT_FALSE(manager.CanLayersCollide(5, 10));
    TEST_ASSERT_FALSE(manager.CanLayersCollide(10, 5));  // Should be symmetric
}

// ========== Test Runner ==========

void TestCollisionManager::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestRegisterCollider);
    RUN_TEST(TestUnregisterCollider);
    RUN_TEST(TestUnregisterAllColliders);
    RUN_TEST(TestSetLayerCollision);
    RUN_TEST(TestCanLayersCollide);
    RUN_TEST(TestSetDefaultCollisionMatrix);
    RUN_TEST(TestUpdate);
    RUN_TEST(TestClearCallbacks);
    RUN_TEST(TestEdgeCases);
}
