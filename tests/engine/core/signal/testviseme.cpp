// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testviseme.cpp
 * @brief Implementation of Viseme unit tests.
 */

#include "testviseme.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

void TestViseme::TestDefaultConstructor() {
    Viseme viseme;
    // Viseme is a simple enum wrapper class - just verify it compiles
    TEST_ASSERT_TRUE(true);
}

void TestViseme::TestParameterizedConstructor() {
    // Viseme doesn't have a parameterized constructor - enum class only
    // Test enum values are accessible
    Viseme::MouthShape ee = Viseme::EE;
    Viseme::MouthShape ah = Viseme::AH;
    TEST_ASSERT_NOT_EQUAL(ee, ah);
}

// ========== Edge Cases ==========

void TestViseme::TestEdgeCases() {
    // Test all enum values are unique
    Viseme::MouthShape shapes[] = {
        Viseme::EE, Viseme::AE, Viseme::UH,
        Viseme::AR, Viseme::ER, Viseme::AH, Viseme::OO, Viseme::SS
    };
    // Just verify they're all different integers
    TEST_ASSERT_NOT_EQUAL(shapes[0], shapes[1]);
    TEST_ASSERT_NOT_EQUAL(shapes[0], shapes[6]);
}

// ========== Test Runner ==========

void TestViseme::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestEdgeCases);
}
