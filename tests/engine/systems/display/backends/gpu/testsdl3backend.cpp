// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testsdl3backend.cpp
 * @brief Implementation of SDL3Backend unit tests.
 */

#include "testsdl3backend.hpp"

using namespace koilo;

// ========== Constructor Tests ==========

void TestSDL3Backend::TestDefaultConstructor() {
    // SDL3Backend cannot be default-constructed
    TEST_IGNORE_MESSAGE("Stub");
}

void TestSDL3Backend::TestParameterizedConstructor() {
    TEST_IGNORE_MESSAGE("Stub");
}

// ========== Method Tests ==========

void TestSDL3Backend::TestInitialize() {
    TEST_IGNORE_MESSAGE("Stub");
}

void TestSDL3Backend::TestShutdown() {
    TEST_IGNORE_MESSAGE("Stub");
}

void TestSDL3Backend::TestIsInitialized() {
    TEST_IGNORE_MESSAGE("Stub");
}

void TestSDL3Backend::TestGetInfo() {
    TEST_IGNORE_MESSAGE("Stub");
}

void TestSDL3Backend::TestPresent() {
    TEST_IGNORE_MESSAGE("Stub");
}

void TestSDL3Backend::TestClear() {
    TEST_IGNORE_MESSAGE("Stub");
}

void TestSDL3Backend::TestSetTitle() {
    TEST_IGNORE_MESSAGE("Stub");
}

void TestSDL3Backend::TestSetFullscreen() {
    TEST_IGNORE_MESSAGE("Stub");
}

void TestSDL3Backend::TestIsWindowOpen() {
    TEST_IGNORE_MESSAGE("Stub");
}

void TestSDL3Backend::TestProcessEvents() {
    TEST_IGNORE_MESSAGE("Stub");
}

// ========== Edge Cases ==========

void TestSDL3Backend::TestEdgeCases() {
    TEST_IGNORE_MESSAGE("Stub");
}

// ========== Test Runner ==========

void TestSDL3Backend::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestInitialize);
    RUN_TEST(TestShutdown);
    RUN_TEST(TestIsInitialized);
    RUN_TEST(TestGetInfo);
    RUN_TEST(TestPresent);
    RUN_TEST(TestClear);
    RUN_TEST(TestSetTitle);
    RUN_TEST(TestSetFullscreen);
    RUN_TEST(TestIsWindowOpen);
    RUN_TEST(TestProcessEvents);
    RUN_TEST(TestEdgeCases);
}
