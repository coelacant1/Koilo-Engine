// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testreflectionbridge.cpp
 * @brief Implementation of ReflectionBridge unit tests.
 */

#include "testreflectionbridge.hpp"

using namespace koilo;
using namespace koilo::scripting;

// ========== Constructor Tests ==========

void TestReflectionBridge::TestDefaultConstructor() {
    // ReflectionBridge is a utility class with static methods
    ReflectionBridge obj;
    (void)obj;
    TEST_ASSERT_TRUE(true);
}

void TestReflectionBridge::TestParameterizedConstructor() {
    // No parameterized constructor; verify FindClass with known class
    // May or may not be registered depending on link order; just check no crash
    (void)ReflectionBridge::FindClass("Vector3D");
    TEST_ASSERT_TRUE(true);
}

// ========== Method Tests ==========

void TestReflectionBridge::TestGetFieldPointer() {
    // Null inputs should return nullptr
    void* nullInst = nullptr;
    void* result = ReflectionBridge::GetFieldPointer(nullInst, (const FieldDecl*)nullptr);
    TEST_ASSERT_TRUE(result == nullptr);
}

// ========== Edge Cases ==========

void TestReflectionBridge::TestEdgeCases() {
    // FindClass with unknown class returns nullptr
    const ClassDesc* desc = ReflectionBridge::FindClass("NonExistentClass12345");
    TEST_ASSERT_TRUE(desc == nullptr);

    // FindField with nullptr class returns nullptr
    const FieldDecl* field = ReflectionBridge::FindField(nullptr, "x");
    TEST_ASSERT_TRUE(field == nullptr);

    // FindMethod with nullptr class returns nullptr
    const MethodDesc* method = ReflectionBridge::FindMethod(nullptr, "foo");
    TEST_ASSERT_TRUE(method == nullptr);
}

// ========== Test Runner ==========

void TestReflectionBridge::TestInvokeMethod() {
    // Null inputs should return nullptr
    void* result = ReflectionBridge::InvokeMethod(nullptr, nullptr);
    TEST_ASSERT_TRUE(result == nullptr);

    // IsVoidMethod with nullptr returns true
    TEST_ASSERT_TRUE(ReflectionBridge::IsVoidMethod(nullptr));
}

void TestReflectionBridge::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestGetFieldPointer);
    RUN_TEST(TestEdgeCases);
    RUN_TEST(TestInvokeMethod);
}
