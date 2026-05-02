// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testvectorfield2d.cpp
 * @brief Implementation of VectorField2D unit tests.
 */

#include "testvectorfield2d.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

void TestVectorField2D::TestDefaultConstructor() {
    // VectorField2D requires parameters (countX, countY)
    VectorField2D field(10, 10);
    TEST_ASSERT_EQUAL_UINT16(10, field.GetCountX());
    TEST_ASSERT_EQUAL_UINT16(10, field.GetCountY());
}

// ========== Method Tests ==========
// ========== Edge Cases ==========

// ========== Test Runner ==========

void TestVectorField2D::TestParameterizedConstructor() {
    // Test various sizes
    VectorField2D small(5, 5);
    TEST_ASSERT_EQUAL_UINT16(5, small.GetCountX());
    TEST_ASSERT_EQUAL_UINT16(5, small.GetCountY());
    
    VectorField2D large(100, 100);
    TEST_ASSERT_EQUAL_UINT16(100, large.GetCountX());
    TEST_ASSERT_EQUAL_UINT16(100, large.GetCountY());
}

void TestVectorField2D::TestEdgeCases() {
    // Minimum size field
    VectorField2D tiny(1, 1);
    TEST_ASSERT_EQUAL_UINT16(1, tiny.GetCountX());
    TEST_ASSERT_EQUAL_UINT16(1, tiny.GetCountY());
    
    // Operations on tiny field should not crash
    tiny.Boundary();
    tiny.Diffuse(0.1f, 0.016f);
    tiny.Advect(0.016f);
    
    // Large field
    VectorField2D large(256, 256);
    TEST_ASSERT_EQUAL_UINT16(256, large.GetCountX());
}

void TestVectorField2D::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);

    RUN_TEST(TestEdgeCases);

}
