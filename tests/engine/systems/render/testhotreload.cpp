// SPDX-License-Identifier: GPL-3.0-or-later
#include "testhotreload.hpp"
#include <koilo/systems/render/hot_reload.hpp>

using namespace koilo;

namespace TestHotReload {

void TestDefaultState() {
    HotReloadService svc;
    TEST_ASSERT_TRUE(svc.IsEnabled());
    TEST_ASSERT_EQUAL_INT(0, svc.ShaderReloadCount());
    TEST_ASSERT_EQUAL_INT(0, svc.ScriptReloadCount());
    TEST_ASSERT_EQUAL_INT(0, (int)svc.WatchCount());
}

void TestEnableDisable() {
    HotReloadService svc;
    TEST_ASSERT_TRUE(svc.IsEnabled());

    svc.SetEnabled(false);
    TEST_ASSERT_FALSE(svc.IsEnabled());

    svc.SetEnabled(true);
    TEST_ASSERT_TRUE(svc.IsEnabled());
}

void TestPollWithNoWatchers() {
    HotReloadService svc;
    int changed = svc.Poll();
    TEST_ASSERT_EQUAL_INT(0, changed);
}

void TestWatchShadersNullPipeline() {
    HotReloadService svc;
    svc.WatchShaders(nullptr);
    TEST_ASSERT_EQUAL_INT(0, (int)svc.WatchCount());
}

void TestWatchScriptEmptyPath() {
    HotReloadService svc;
    svc.WatchScript("", nullptr);
    TEST_ASSERT_EQUAL_INT(0, (int)svc.WatchCount());
}

void TestWatchScriptNullEngine() {
    HotReloadService svc;
    svc.WatchScript("/tmp/test.ks", nullptr);
    TEST_ASSERT_EQUAL_INT(0, (int)svc.WatchCount());
}

void RunAllTests() {
    RUN_TEST(TestDefaultState);
    RUN_TEST(TestEnableDisable);
    RUN_TEST(TestPollWithNoWatchers);
    RUN_TEST(TestWatchShadersNullPipeline);
    RUN_TEST(TestWatchScriptEmptyPath);
    RUN_TEST(TestWatchScriptNullEngine);
}

} // namespace TestHotReload
