/**
 * @file testustring.cpp
 * @brief Implementation of UString unit tests.
 */

#include "testustring.hpp"

using namespace ptx;

// ========== Constructor Tests ==========

void TestUString::TestDefaultConstructor() {
    UString str;

    TEST_ASSERT_TRUE(str.IsEmpty());
    TEST_ASSERT_EQUAL(0, str.Length());
    TEST_ASSERT_EQUAL_STRING("", str.CStr());
}

// ========== Assignment Operator Tests ==========

// ========== Append Tests ==========

// ========== Concatenation Operator Tests ==========

// ========== Length and Empty Tests ==========

void TestUString::TestLength() {
    UString empty;
    UString short_str("Hi");
    UString long_str("This is a longer string");

    TEST_ASSERT_EQUAL(0, empty.Length());
    TEST_ASSERT_EQUAL(2, short_str.Length());
    TEST_ASSERT_EQUAL(23, long_str.Length());
}

void TestUString::TestIsEmpty() {
    UString empty;
    UString not_empty("Text");

    TEST_ASSERT_TRUE(empty.IsEmpty());
    TEST_ASSERT_FALSE(not_empty.IsEmpty());
}

// ========== Clear Tests ==========

void TestUString::TestClear() {
    UString str("Some content here");

    TEST_ASSERT_FALSE(str.IsEmpty());
    TEST_ASSERT_EQUAL(17, str.Length());

    str.Clear();

    TEST_ASSERT_TRUE(str.IsEmpty());
    TEST_ASSERT_EQUAL(0, str.Length());
    TEST_ASSERT_EQUAL_STRING("", str.CStr());
}

// ========== CStr Tests ==========

void TestUString::TestCStr() {
    UString str("TestString");

    const char* c_str = str.CStr();

    TEST_ASSERT_NOT_NULL(c_str);
    TEST_ASSERT_EQUAL_STRING("TestString", c_str);
}

// ========== FromFloat Tests ==========

// ========== Edge Cases ==========

// ========== Test Runner ==========

void TestUString::TestAppend() {
    // TODO: Implement test for Append()
    // UString obj; // Requires constructor parameters
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestUString::TestEdgeCases() {
    // TODO: Test edge cases (null, boundaries, extreme values)
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestUString::TestParameterizedConstructor() {
    // TODO: Implement test for parameterized constructor
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestUString::RunAllTests() {
    // Constructor tests
    RUN_TEST(TestDefaultConstructor);

    // Assignment tests

    // Append tests

    // Concatenation operator tests

    // Length and empty tests
    RUN_TEST(TestLength);
    RUN_TEST(TestIsEmpty);

    // Clear tests
    RUN_TEST(TestClear);

    // CStr tests
    RUN_TEST(TestCStr);

    // FromFloat tests

    // Edge cases

    RUN_TEST(TestAppend);
    RUN_TEST(TestEdgeCases);
    RUN_TEST(TestParameterizedConstructor);
}
