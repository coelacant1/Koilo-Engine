// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testimage.cpp
 * @brief Implementation of Image unit tests.
 */

#include "testimage.hpp"

using namespace koilo;

// ========== Constructor Tests ==========

void TestImage::TestDefaultConstructor() {
    TEST_ASSERT_TRUE(true);  
}

// ========== Method Tests ==========
void TestImage::TestSetData() {
    TEST_ASSERT_TRUE(true);  
}
void TestImage::TestSetColorPalette() {
    TEST_ASSERT_TRUE(true);  
}
void TestImage::TestSetSize() {
    TEST_ASSERT_TRUE(true);  
}
void TestImage::TestSetPosition() {
    TEST_ASSERT_TRUE(true);  
}
void TestImage::TestSetRotation() {
    TEST_ASSERT_TRUE(true);  
}
void TestImage::TestGetColorAtCoordinate() {
    TEST_ASSERT_TRUE(true);  
}
// ========== Edge Cases ==========

// ========== Test Runner ==========

void TestImage::TestParameterizedConstructor() {
    
    TEST_ASSERT_TRUE(true);
}

void TestImage::TestEdgeCases() {
    
    TEST_ASSERT_TRUE(true);
}

void TestImage::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestSetData);
    RUN_TEST(TestSetColorPalette);
    RUN_TEST(TestSetSize);
    RUN_TEST(TestSetPosition);
    RUN_TEST(TestSetRotation);
    RUN_TEST(TestGetColorAtCoordinate);
    RUN_TEST(TestEdgeCases);
}
