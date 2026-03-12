// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testlocalization.cpp
 * @brief Tests for Localization string table.
 * @date 03/09/2026
 * @author Coela
 */

#include "testlocalization.hpp"
#include <cstring>

using namespace koilo;

void TestLocalization::TestDefaultLocale() {
    Localization loc;
    TEST_ASSERT_EQUAL_STRING("en", loc.GetLocale());
}

void TestLocalization::TestSetGet() {
    Localization loc;
    TEST_ASSERT_TRUE(loc.Set("greeting", "Hello"));
    TEST_ASSERT_EQUAL_STRING("Hello", loc.Get("greeting"));
}

void TestLocalization::TestFallbackToKey() {
    Localization loc;
    // Key not set - returns the key itself
    TEST_ASSERT_EQUAL_STRING("missing_key", loc.Get("missing_key"));
}

void TestLocalization::TestUpdateExisting() {
    Localization loc;
    loc.Set("title", "Old Title");
    loc.Set("title", "New Title");
    TEST_ASSERT_EQUAL_STRING("New Title", loc.Get("title"));
}

void TestLocalization::TestClear() {
    Localization loc;
    loc.Set("a", "A");
    loc.Set("b", "B");
    TEST_ASSERT_EQUAL_UINT(2, loc.Count());
    loc.Clear();
    TEST_ASSERT_EQUAL_UINT(0, loc.Count());
    TEST_ASSERT_EQUAL_STRING("a", loc.Get("a")); // fallback
}

void TestLocalization::TestRTL() {
    Localization loc;
    TEST_ASSERT_FALSE(loc.IsRTL());
    loc.SetLocale("ar");
    loc.SetRTL(true);
    TEST_ASSERT_TRUE(loc.IsRTL());
    TEST_ASSERT_EQUAL_STRING("ar", loc.GetLocale());
}

void TestLocalization::TestFontScale() {
    Localization loc;
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, loc.FontScale());
    loc.SetFontScale(1.5f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.5f, loc.FontScale());
    // Clamp to minimum 0.1
    loc.SetFontScale(0.01f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.1f, loc.FontScale());
}

void TestLocalization::TestCount() {
    Localization loc;
    TEST_ASSERT_EQUAL_UINT(0, loc.Count());
    loc.Set("k1", "v1");
    loc.Set("k2", "v2");
    TEST_ASSERT_EQUAL_UINT(2, loc.Count());
}

void TestLocalization::TestLAlias() {
    Localization loc;
    loc.Set("ok", "OK");
    TEST_ASSERT_EQUAL_STRING("OK", loc.L("ok"));
    TEST_ASSERT_EQUAL_STRING("nope", loc.L("nope")); // fallback
}

void TestLocalization::RunAllTests() {
    RUN_TEST(TestLocalization::TestDefaultLocale);
    RUN_TEST(TestLocalization::TestSetGet);
    RUN_TEST(TestLocalization::TestFallbackToKey);
    RUN_TEST(TestLocalization::TestUpdateExisting);
    RUN_TEST(TestLocalization::TestClear);
    RUN_TEST(TestLocalization::TestRTL);
    RUN_TEST(TestLocalization::TestFontScale);
    RUN_TEST(TestLocalization::TestCount);
    RUN_TEST(TestLocalization::TestLAlias);
}
