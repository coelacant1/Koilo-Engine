// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testcomponent.cpp
 * @brief Implementation of Component unit tests.
 *
 * @date 24/10/2025
 * @author Coela
 */

#include "testcomponent.hpp"
#include <koilo/ecs/component.hpp>

using namespace koilo;

// Test component types
struct TestComponentA { int value; };
struct TestComponentB { float data; };
struct TestComponentC { double x, y; };

void TestComponent::TestComponentTypeIDGeneration() {
    ComponentTypeID idA = GetComponentTypeID<TestComponentA>();
    TEST_ASSERT_TRUE(idA >= 0);
}

void TestComponent::TestComponentTypeIDUniqueness() {
    ComponentTypeID idA = GetComponentTypeID<TestComponentA>();
    ComponentTypeID idB = GetComponentTypeID<TestComponentB>();
    ComponentTypeID idC = GetComponentTypeID<TestComponentC>();
    
    TEST_ASSERT_NOT_EQUAL(idA, idB);
    TEST_ASSERT_NOT_EQUAL(idA, idC);
    TEST_ASSERT_NOT_EQUAL(idB, idC);
}

void TestComponent::TestComponentTypeIDConsistency() {
    ComponentTypeID idA1 = GetComponentTypeID<TestComponentA>();
    ComponentTypeID idA2 = GetComponentTypeID<TestComponentA>();
    ComponentTypeID idA3 = GetComponentTypeID<TestComponentA>();
    
    TEST_ASSERT_EQUAL(idA1, idA2);
    TEST_ASSERT_EQUAL(idA1, idA3);
}

void TestComponent::TestComponentTypeIDCount() {
    ComponentTypeID countBefore = ComponentTypeIDGenerator::GetCount();
    TEST_ASSERT_TRUE(countBefore >= 0);
    
    // Note: Cannot reliably test increment since IDs are generated statically
    // Just verify count is consistent
    ComponentTypeID countAfter = ComponentTypeIDGenerator::GetCount();
    TEST_ASSERT_EQUAL(countBefore, countAfter);
}

void TestComponent::TestMultipleComponentTypes() {
    // Generate IDs for multiple types
    ComponentTypeID id1 = GetComponentTypeID<TestComponentA>();
    ComponentTypeID id2 = GetComponentTypeID<TestComponentB>();
    ComponentTypeID id3 = GetComponentTypeID<TestComponentC>();
    
    // Verify all are different
    TEST_ASSERT_TRUE(id1 != id2 && id1 != id3 && id2 != id3);
    
    // Verify consistency on second call
    TEST_ASSERT_EQUAL(id1, GetComponentTypeID<TestComponentA>());
    TEST_ASSERT_EQUAL(id2, GetComponentTypeID<TestComponentB>());
    TEST_ASSERT_EQUAL(id3, GetComponentTypeID<TestComponentC>());
}

void TestComponent::RunAllTests() {
    RUN_TEST(TestComponentTypeIDGeneration);
    RUN_TEST(TestComponentTypeIDUniqueness);
    RUN_TEST(TestComponentTypeIDConsistency);
    RUN_TEST(TestComponentTypeIDCount);
    RUN_TEST(TestMultipleComponentTypes);
}
