// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testconsole.cpp
 * @brief Implementation of Console tests.
 *
 * @date 24/10/2025
 * @author Coela
 */

#include "testconsole.hpp"
#include <koilo/core/platform/console.hpp>

using namespace koilo;

void TestConsole::TestBegin() {
    // Should not crash
    Console::Begin(9600);
    TEST_ASSERT_TRUE(true);  // Console output test
}

void TestConsole::TestPrintString() {
    Console::Print("Test");
    TEST_ASSERT_TRUE(true);  // Console output test
}

void TestConsole::TestPrintInt() {
    Console::Print(42);
    TEST_ASSERT_TRUE(true);  // Console output test
}

void TestConsole::TestPrintFloat() {
    Console::Print(3.14f, 2);
    TEST_ASSERT_TRUE(true);  // Console output test
}

void TestConsole::TestPrintlnEmpty() {
    Console::Println();
    TEST_ASSERT_TRUE(true);  // Console output test
}

void TestConsole::TestPrintlnString() {
    Console::Println("Test");
    TEST_ASSERT_TRUE(true);  // Console output test
}

void TestConsole::TestPrintlnInt() {
    Console::Println(42);
    TEST_ASSERT_TRUE(true);  // Console output test
}

void TestConsole::TestPrintlnFloat() {
    Console::Println(3.14f, 2);
    TEST_ASSERT_TRUE(true);  // Console output test
}

void TestConsole::RunAllTests() {
    RUN_TEST(TestBegin);
    RUN_TEST(TestPrintString);
    RUN_TEST(TestPrintInt);
    RUN_TEST(TestPrintFloat);
    RUN_TEST(TestPrintlnEmpty);
    RUN_TEST(TestPrintlnString);
    RUN_TEST(TestPrintlnInt);
    RUN_TEST(TestPrintlnFloat);
}
