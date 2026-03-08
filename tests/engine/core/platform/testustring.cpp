// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testustring.cpp
 * @brief Implementation of UString unit tests.
 */

#include "testustring.hpp"

using namespace koilo;

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
    UString str("Hello");
    str += " World";
    TEST_ASSERT_EQUAL_STRING("Hello World", str.CStr());
    
    UString str2("Foo");
    UString append(" Bar");
    str2 += append;
    TEST_ASSERT_EQUAL_STRING("Foo Bar", str2.CStr());
}

void TestUString::TestEdgeCases() {
    UString empty1;
    UString empty2("");
    TEST_ASSERT_TRUE(empty1.IsEmpty());
    TEST_ASSERT_TRUE(empty2.IsEmpty());
    
    UString nullStr(nullptr);
    TEST_ASSERT_TRUE(nullStr.IsEmpty());
    TEST_ASSERT_EQUAL(0, nullStr.Length());
    
    UString veryLong("This is a very long string that should test memory allocation and handling of larger strings without issues");
    TEST_ASSERT_TRUE(veryLong.Length() > 50);
    TEST_ASSERT_FALSE(veryLong.IsEmpty());
}

void TestUString::TestParameterizedConstructor() {
    UString str1("Test");
    TEST_ASSERT_EQUAL_STRING("Test", str1.CStr());
    TEST_ASSERT_EQUAL(4, str1.Length());
    
    UString str2("Another String");
    TEST_ASSERT_EQUAL_STRING("Another String", str2.CStr());
    TEST_ASSERT_EQUAL(14, str2.Length());
    
    UString copy(str2);
    TEST_ASSERT_EQUAL_STRING(str2.CStr(), copy.CStr());
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
