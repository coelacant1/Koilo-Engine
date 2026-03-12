// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testfilewatcher.cpp
 * @brief Unit tests for the FileWatcher.
 */

#include "testfilewatcher.hpp"
#include <koilo/core/platform/file_watcher.hpp>
#include <cstdio>
#include <cstring>
#include <unistd.h>

using namespace koilo;

static const char* kTmpFile = "/tmp/koilo_fw_test.txt";

static void WriteTmp(const char* content) {
    FILE* f = fopen(kTmpFile, "w");
    if (f) { fputs(content, f); fclose(f); }
}

void TestFileWatcher::TestWatchCount() {
    FileWatcher fw;
    TEST_ASSERT_EQUAL(0u, fw.Count());
    WriteTmp("a");
    fw.Watch(kTmpFile);
    TEST_ASSERT_EQUAL(1u, fw.Count());
    // Duplicate should not increase count
    fw.Watch(kTmpFile);
    TEST_ASSERT_EQUAL(1u, fw.Count());
}

void TestFileWatcher::TestUnwatch() {
    FileWatcher fw;
    WriteTmp("b");
    fw.Watch(kTmpFile);
    fw.Unwatch(kTmpFile);
    TEST_ASSERT_EQUAL(0u, fw.Count());
}

void TestFileWatcher::TestPollNoChange() {
    FileWatcher fw;
    WriteTmp("c");
    fw.Watch(kTmpFile);
    int changes = fw.Poll();
    TEST_ASSERT_EQUAL(0, changes);
}

void TestFileWatcher::TestPollDetectsChange() {
    FileWatcher fw;
    WriteTmp("d");
    fw.Watch(kTmpFile);

    // Wait for mtime granularity then modify
    sleep(1);
    WriteTmp("e");

    int callbackCount = 0;
    fw.SetCallback([&](const std::string&) { callbackCount++; });
    int changes = fw.Poll();
    TEST_ASSERT_EQUAL(1, changes);
    TEST_ASSERT_EQUAL(1, callbackCount);

    // Subsequent poll with no change should return 0
    changes = fw.Poll();
    TEST_ASSERT_EQUAL(0, changes);
}

void TestFileWatcher::TestClear() {
    FileWatcher fw;
    WriteTmp("f");
    fw.Watch(kTmpFile);
    fw.Clear();
    TEST_ASSERT_EQUAL(0u, fw.Count());
}

void TestFileWatcher::RunAllTests() {
    RUN_TEST(TestFileWatcher::TestWatchCount);
    RUN_TEST(TestFileWatcher::TestUnwatch);
    RUN_TEST(TestFileWatcher::TestPollNoChange);
    RUN_TEST(TestFileWatcher::TestPollDetectsChange);
    RUN_TEST(TestFileWatcher::TestClear);
}
