// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testphysicsmaterial.cpp
 * @brief Implementation of PhysicsMaterial unit tests.
 */

#include "testphysicsmaterial.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

void TestPhysicsMaterial::TestDefaultConstructor() {
    PhysicsMaterial material;
    // Verify default values
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.5f, material.friction);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.3f, material.bounciness);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, material.density);
}

void TestPhysicsMaterial::TestParameterizedConstructor() {
    PhysicsMaterial material(0.8f, 0.6f, 2.5f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.8f, material.friction);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.6f, material.bounciness);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 2.5f, material.density);
}

// ========== Edge Cases ==========

void TestPhysicsMaterial::TestEdgeCases() {
    // Zero friction and bounce (ice/perfect slide)
    PhysicsMaterial ice(0.0f, 0.0f, 0.9f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, ice.friction);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, ice.bounciness);
    
    // Maximum friction and bounce (rubber/sticky)
    PhysicsMaterial rubber(1.0f, 0.9f, 1.2f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, rubber.friction);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.9f, rubber.bounciness);
    
    // High density (metal)
    PhysicsMaterial metal(0.4f, 0.1f, 7.8f);  // Steel ~7.85 g/cm³
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 7.8f, metal.density);
}

// ========== Test Runner ==========

void TestPhysicsMaterial::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestEdgeCases);
}
