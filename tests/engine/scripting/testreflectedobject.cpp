// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testreflectedobject.cpp
 * @brief Implementation of ReflectedObject unit tests.
 */

#include "testreflectedobject.hpp"

using namespace koilo;
using namespace koilo::scripting;

// ========== Constructor Tests ==========

void TestReflectedObject::TestDefaultConstructor() {
    ReflectedObject obj;
    TEST_ASSERT_TRUE(obj.instance == nullptr);
    TEST_ASSERT_TRUE(obj.classDesc == nullptr);
    TEST_ASSERT_EQUAL_STRING("", obj.id.c_str());
    TEST_ASSERT_TRUE(obj.ownsInstance);
}

void TestReflectedObject::TestParameterizedConstructor() {
    int dummy = 42;
    ReflectedObject obj(&dummy, nullptr, "testObj", false);
    TEST_ASSERT_TRUE(obj.instance == &dummy);
    TEST_ASSERT_TRUE(obj.classDesc == nullptr);
    TEST_ASSERT_EQUAL_STRING("testObj", obj.id.c_str());
    TEST_ASSERT_FALSE(obj.ownsInstance);
}

// ========== Edge Cases ==========

void TestReflectedObject::TestEdgeCases() {
    // Null instance with valid id
    ReflectedObject obj(nullptr, nullptr, "empty", true);
    TEST_ASSERT_TRUE(obj.instance == nullptr);
    TEST_ASSERT_EQUAL_STRING("empty", obj.id.c_str());
    TEST_ASSERT_TRUE(obj.ownsInstance);
}

// ========== Test Runner ==========

void TestReflectedObject::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestEdgeCases);
}
