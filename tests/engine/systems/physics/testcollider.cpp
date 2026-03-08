// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testcollider.cpp
 * @brief Implementation of base Collider tests using concrete BoxCollider.
 *
 * @date 24/10/2025
 * @author Coela
 */

#include "testcollider.hpp"
#include <koilo/systems/physics/boxcollider.hpp>

using namespace koilo;

void TestCollider::TestContainsPoint() {
    // TODO: Implement test for ContainsPoint()
    // Collider is abstract
    TEST_IGNORE_MESSAGE("Stub");
}

void TestCollider::TestDefaultConstructor() {
    // TODO: Implement test for default constructor
    // Collider is abstract
    TEST_IGNORE_MESSAGE("Stub");
}

void TestCollider::TestEdgeCases() {
    // TODO: Test edge cases (null, boundaries, extreme values)
    TEST_IGNORE_MESSAGE("Stub");
}

void TestCollider::TestGetLayer() {
    // TODO: Implement test for GetLayer()
    // Collider is abstract
    TEST_IGNORE_MESSAGE("Stub");
}

void TestCollider::TestGetMaterial() {
    // TODO: Implement test for GetMaterial()
    // Collider is abstract
    TEST_IGNORE_MESSAGE("Stub");
}

void TestCollider::TestGetTag() {
    // TODO: Implement test for GetTag()
    // Collider is abstract
    TEST_IGNORE_MESSAGE("Stub");
}

void TestCollider::TestGetType() {
    // TODO: Implement test for GetType()
    // Collider is abstract
    TEST_IGNORE_MESSAGE("Stub");
}

void TestCollider::TestIsEnabled() {
    // TODO: Implement test for IsEnabled()
    // Collider is abstract
    TEST_IGNORE_MESSAGE("Stub");
}

void TestCollider::TestIsTrigger() {
    // TODO: Implement test for IsTrigger()
    // Collider is abstract
    TEST_IGNORE_MESSAGE("Stub");
}

void TestCollider::TestParameterizedConstructor() {
    // TODO: Implement test for parameterized constructor
    TEST_IGNORE_MESSAGE("Stub");
}

void TestCollider::TestRaycastHitResult() {
    // TODO: Implement test for RaycastHitResult()
    // Collider is abstract
    TEST_IGNORE_MESSAGE("Stub");
}

void TestCollider::TestSetEnabled() {
    // TODO: Implement test for SetEnabled()
    // Collider is abstract
    TEST_IGNORE_MESSAGE("Stub");
}

void TestCollider::TestSetLayer() {
    // TODO: Implement test for SetLayer()
    // Collider is abstract
    TEST_IGNORE_MESSAGE("Stub");
}

void TestCollider::TestSetMaterial() {
    // TODO: Implement test for SetMaterial()
    // Collider is abstract
    TEST_IGNORE_MESSAGE("Stub");
}

void TestCollider::TestSetTag() {
    // TODO: Implement test for SetTag()
    // Collider is abstract
    TEST_IGNORE_MESSAGE("Stub");
}

void TestCollider::TestSetTrigger() {
    // TODO: Implement test for SetTrigger()
    // Collider is abstract
    TEST_IGNORE_MESSAGE("Stub");
}

void TestCollider::RunAllTests() {

    RUN_TEST(TestContainsPoint);
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestEdgeCases);
    RUN_TEST(TestGetLayer);
    RUN_TEST(TestGetMaterial);
    RUN_TEST(TestGetTag);
    RUN_TEST(TestGetType);
    RUN_TEST(TestIsEnabled);
    RUN_TEST(TestIsTrigger);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestRaycastHitResult);
    RUN_TEST(TestSetEnabled);
    RUN_TEST(TestSetLayer);
    RUN_TEST(TestSetMaterial);
    RUN_TEST(TestSetTag);
    RUN_TEST(TestSetTrigger);
}
