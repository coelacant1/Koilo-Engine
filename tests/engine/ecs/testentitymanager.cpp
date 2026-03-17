// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testentitymanager.cpp
 * @brief Implementation of EntityManager unit tests.
 *
 * @date 24/10/2025
 * @author Coela
 */

#include "testentitymanager.hpp"
#include <koilo/systems/ecs/entitymanager.hpp>
#include <koilo/systems/ecs/entity.hpp>

using namespace koilo;

// Test component types
struct PositionComponent {
    float x, y, z;
    PositionComponent(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
};

struct VelocityComponent {
    float vx, vy, vz;
    VelocityComponent(float vx = 0, float vy = 0, float vz = 0) : vx(vx), vy(vy), vz(vz) {}
};

struct HealthComponent {
    int health;
    HealthComponent(int h = 100) : health(h) {}
};

// === Entity Tests ===

// === EntityManager - Basic Operations ===

void TestEntityManager::TestCreateEntity() {
    EntityManager manager;
    
    Entity e1 = manager.CreateEntity();
    TEST_ASSERT_TRUE(e1.IsValid());
    TEST_ASSERT_TRUE(manager.IsEntityValid(e1));
    TEST_ASSERT_EQUAL(0, e1.GetIndex());
    TEST_ASSERT_EQUAL(1, e1.GetGeneration());
}

void TestEntityManager::TestDestroyEntity() {
    EntityManager manager;
    
    Entity e1 = manager.CreateEntity();
    TEST_ASSERT_TRUE(manager.IsEntityValid(e1));
    
    manager.DestroyEntity(e1);
    TEST_ASSERT_FALSE(manager.IsEntityValid(e1));
}

void TestEntityManager::TestIsEntityValid() {
    EntityManager manager;
    
    Entity e1 = manager.CreateEntity();
    TEST_ASSERT_TRUE(manager.IsEntityValid(e1));
    
    manager.DestroyEntity(e1);
    TEST_ASSERT_FALSE(manager.IsEntityValid(e1));
    
    Entity e2;
    TEST_ASSERT_FALSE(manager.IsEntityValid(e2));
}

// === EntityManager - Component Operations ===

// === EntityManager - Queries ===

// === EntityManager - Edge Cases ===

void TestEntityManager::TestClear() {
    EntityManager manager;
    
    Entity e1 = manager.CreateEntity();
    Entity e2 = manager.CreateEntity();
    
    manager.AddComponent(e1, PositionComponent());
    manager.AddComponent(e2, VelocityComponent());
    
    manager.Clear();
    
    TEST_ASSERT_EQUAL(0, manager.GetEntityCount());
    TEST_ASSERT_FALSE(manager.IsEntityValid(e1));
    TEST_ASSERT_FALSE(manager.IsEntityValid(e2));
}

void TestEntityManager::TestDefaultConstructor() {
    // TODO: Implement test for default constructor
    EntityManager obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestEntityManager::TestEdgeCases() {
    // TODO: Test edge cases (null, boundaries, extreme values)
    TEST_IGNORE_MESSAGE("Stub");
}

void TestEntityManager::TestGetEntityCount() {
    // TODO: Implement test for GetEntityCount()
    EntityManager obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestEntityManager::TestParameterizedConstructor() {
    // TODO: Implement test for parameterized constructor
    TEST_IGNORE_MESSAGE("Stub");
}

void TestEntityManager::RunAllTests() {

    RUN_TEST(TestCreateEntity);
    RUN_TEST(TestDestroyEntity);

    RUN_TEST(TestIsEntityValid);

    RUN_TEST(TestClear);

    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestEdgeCases);
    RUN_TEST(TestGetEntityCount);
    RUN_TEST(TestParameterizedConstructor);
}
