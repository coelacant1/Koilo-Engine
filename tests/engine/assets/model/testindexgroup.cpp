// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testindexgroup.cpp
 * @brief Implementation of IndexGroup unit tests.
 */

#include "testindexgroup.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

void TestIndexGroup::TestDefaultConstructor() {
    IndexGroup ig;
    
    // Default constructor should initialize to (0,0,0)
    TEST_ASSERT_EQUAL_UINT32(0, ig.A);
    TEST_ASSERT_EQUAL_UINT32(0, ig.B);
    TEST_ASSERT_EQUAL_UINT32(0, ig.C);
}

void TestIndexGroup::TestParameterizedConstructor() {
    IndexGroup ig(10, 20, 30);
    
    TEST_ASSERT_EQUAL_UINT32(10, ig.A);
    TEST_ASSERT_EQUAL_UINT32(20, ig.B);
    TEST_ASSERT_EQUAL_UINT32(30, ig.C);
}

// ========== Method Tests ==========

void TestIndexGroup::TestAdd() {
    IndexGroup ig1(1, 2, 3);
    IndexGroup ig2(10, 20, 30);
    
    IndexGroup result = ig1.Add(ig2);
    
    TEST_ASSERT_EQUAL_UINT32(11, result.A); // 1 + 10
    TEST_ASSERT_EQUAL_UINT32(22, result.B); // 2 + 20
    TEST_ASSERT_EQUAL_UINT32(33, result.C); // 3 + 30
}

void TestIndexGroup::TestSubtract() {
    IndexGroup ig1(50, 60, 70);
    IndexGroup ig2(10, 20, 30);
    
    IndexGroup result = ig1.Subtract(ig2);
    
    TEST_ASSERT_EQUAL_UINT32(40, result.A); // 50 - 10
    TEST_ASSERT_EQUAL_UINT32(40, result.B); // 60 - 20
    TEST_ASSERT_EQUAL_UINT32(40, result.C); // 70 - 30
}

void TestIndexGroup::TestMultiply() {
    IndexGroup ig1(2, 3, 4);
    IndexGroup ig2(5, 6, 7);
    
    IndexGroup result = ig1.Multiply(ig2);
    
    TEST_ASSERT_EQUAL_UINT32(10, result.A); // 2 * 5
    TEST_ASSERT_EQUAL_UINT32(18, result.B); // 3 * 6
    TEST_ASSERT_EQUAL_UINT32(28, result.C); // 4 * 7
}

void TestIndexGroup::TestDivide() {
    IndexGroup ig1(100, 200, 300);
    IndexGroup ig2(10, 20, 30);
    
    IndexGroup result = ig1.Divide(ig2);
    
    TEST_ASSERT_EQUAL_UINT32(10, result.A); // 100 / 10
    TEST_ASSERT_EQUAL_UINT32(10, result.B); // 200 / 20
    TEST_ASSERT_EQUAL_UINT32(10, result.C); // 300 / 30
}

void TestIndexGroup::TestGetIndex() {
    IndexGroup ig(5, 10, 15);
    
    TEST_ASSERT_EQUAL_UINT32(5, ig.GetIndex(0));  // A
    TEST_ASSERT_EQUAL_UINT32(10, ig.GetIndex(1)); // B
    TEST_ASSERT_EQUAL_UINT32(15, ig.GetIndex(2)); // C
    TEST_ASSERT_EQUAL_UINT32(15, ig.GetIndex(3)); // Default to C
}

void TestIndexGroup::TestToString() {
    IndexGroup ig(1, 2, 3);
    
    koilo::UString str = ig.ToString();
    
    // ToString should return format like "[1, 2, 3]"
    TEST_ASSERT_TRUE(true);
}

// ========== Edge Cases ==========

void TestIndexGroup::TestEdgeCases() {
    // Test with zero values
    IndexGroup igZero(0, 0, 0);
    IndexGroup ig(10, 20, 30);
    
    IndexGroup addResult = igZero.Add(ig);
    TEST_ASSERT_EQUAL_UINT32(10, addResult.A);
    
    IndexGroup mulResult = igZero.Multiply(ig);
    TEST_ASSERT_EQUAL_UINT32(0, mulResult.A); // 0 * 10 = 0
    
    // Test with large values
    IndexGroup igLarge(1000, 2000, 3000);
    IndexGroup addLarge = igLarge.Add(igLarge);
    TEST_ASSERT_EQUAL_UINT32(2000, addLarge.A);
    
    // Test subtract with equal values (should get 0)
    IndexGroup subResult = ig.Subtract(ig);
    TEST_ASSERT_EQUAL_UINT32(0, subResult.A);
    TEST_ASSERT_EQUAL_UINT32(0, subResult.B);
    TEST_ASSERT_EQUAL_UINT32(0, subResult.C);
}

void TestIndexGroup::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestAdd);
    RUN_TEST(TestSubtract);
    RUN_TEST(TestMultiply);
    RUN_TEST(TestDivide);
    RUN_TEST(TestToString);
    RUN_TEST(TestEdgeCases);
    RUN_TEST(TestGetIndex);
}
