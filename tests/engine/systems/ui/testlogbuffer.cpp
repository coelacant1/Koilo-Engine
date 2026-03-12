// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testlogbuffer.cpp
 * @brief Tests for the ring-buffer log system.
 * @date 03/09/2026
 * @author Coela
 */

#include "testlogbuffer.hpp"
#include <cstring>
#include <cstdio>

void TestLogBuffer::TestInitiallyEmpty() {
    koilo::LogBuffer buf;
    TEST_ASSERT_EQUAL_UINT(0, static_cast<unsigned>(buf.Count()));
    TEST_ASSERT_EQUAL_UINT(0, static_cast<unsigned>(buf.TotalPushed()));
}

void TestLogBuffer::TestPushAndRead() {
    koilo::LogBuffer buf;
    buf.Push(koilo::LogSeverity::Info, "hello world", 1);
    TEST_ASSERT_EQUAL_UINT(1, static_cast<unsigned>(buf.Count()));
    const koilo::LogEntry* e = buf.At(0);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(koilo::LogSeverity::Info),
                          static_cast<int>(e->severity));
    TEST_ASSERT_EQUAL_STRING("hello world", e->message);
    TEST_ASSERT_EQUAL_UINT32(1, e->timestamp);
}

void TestLogBuffer::TestRingBufferWrap() {
    koilo::LogBuffer buf(4);
    for (int i = 0; i < 6; ++i) {
        char msg[32];
        snprintf(msg, sizeof(msg), "msg%d", i);
        buf.Push(koilo::LogSeverity::Info, msg, static_cast<uint32_t>(i));
    }
    TEST_ASSERT_EQUAL_UINT(4, static_cast<unsigned>(buf.Count()));
    TEST_ASSERT_EQUAL_UINT(6, static_cast<unsigned>(buf.TotalPushed()));
    TEST_ASSERT_EQUAL_STRING("msg2", buf.At(0)->message);
    TEST_ASSERT_EQUAL_STRING("msg5", buf.At(3)->message);
}

void TestLogBuffer::TestOutOfBounds() {
    koilo::LogBuffer buf;
    buf.Push(koilo::LogSeverity::Info, "test");
    TEST_ASSERT_NULL(buf.At(1));
    TEST_ASSERT_NULL(buf.At(999));
}

void TestLogBuffer::TestClear() {
    koilo::LogBuffer buf;
    buf.Push(koilo::LogSeverity::Info, "a");
    buf.Push(koilo::LogSeverity::Warning, "b");
    buf.Clear();
    TEST_ASSERT_EQUAL_UINT(0, static_cast<unsigned>(buf.Count()));
}

void TestLogBuffer::TestCountBySeverity() {
    koilo::LogBuffer buf;
    buf.Push(koilo::LogSeverity::Info, "info1");
    buf.Push(koilo::LogSeverity::Warning, "warn1");
    buf.Push(koilo::LogSeverity::Error, "err1");
    buf.Push(koilo::LogSeverity::Info, "info2");
    buf.Push(koilo::LogSeverity::Debug, "dbg1");
    TEST_ASSERT_EQUAL_UINT(2, static_cast<unsigned>(buf.CountBySeverity(koilo::LogSeverity::Info)));
    TEST_ASSERT_EQUAL_UINT(1, static_cast<unsigned>(buf.CountBySeverity(koilo::LogSeverity::Warning)));
    TEST_ASSERT_EQUAL_UINT(1, static_cast<unsigned>(buf.CountBySeverity(koilo::LogSeverity::Error)));
    TEST_ASSERT_EQUAL_UINT(1, static_cast<unsigned>(buf.CountBySeverity(koilo::LogSeverity::Debug)));
}

void TestLogBuffer::TestSearchCI() {
    koilo::LogBuffer buf;
    buf.Push(koilo::LogSeverity::Info, "Player connected");
    buf.Push(koilo::LogSeverity::Warning, "PLAYER disconnected");
    buf.Push(koilo::LogSeverity::Error, "Server crashed");
    size_t indices[16];
    size_t found = buf.Search("player", indices, 16);
    TEST_ASSERT_EQUAL_UINT(2, static_cast<unsigned>(found));
}

void TestLogBuffer::TestSearchEmpty() {
    koilo::LogBuffer buf;
    buf.Push(koilo::LogSeverity::Info, "a");
    buf.Push(koilo::LogSeverity::Info, "b");
    size_t indices[16];
    size_t found = buf.Search("", indices, 16);
    TEST_ASSERT_EQUAL_UINT(2, static_cast<unsigned>(found));
}

void TestLogBuffer::TestMessageTruncation() {
    koilo::LogBuffer buf;
    char longMsg[512];
    memset(longMsg, 'x', 511);
    longMsg[511] = '\0';
    buf.Push(koilo::LogSeverity::Info, longMsg);
    const koilo::LogEntry* e = buf.At(0);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_TRUE(strlen(e->message) < koilo::LogEntry::MAX_MSG_LEN);
}

void TestLogBuffer::TestTimestamp() {
    koilo::LogBuffer buf;
    buf.Push(koilo::LogSeverity::Debug, "frame", 42);
    TEST_ASSERT_EQUAL_UINT32(42, buf.At(0)->timestamp);
}

void TestLogBuffer::RunAllTests() {
    RUN_TEST(TestLogBuffer::TestInitiallyEmpty);
    RUN_TEST(TestLogBuffer::TestPushAndRead);
    RUN_TEST(TestLogBuffer::TestRingBufferWrap);
    RUN_TEST(TestLogBuffer::TestOutOfBounds);
    RUN_TEST(TestLogBuffer::TestClear);
    RUN_TEST(TestLogBuffer::TestCountBySeverity);
    RUN_TEST(TestLogBuffer::TestSearchCI);
    RUN_TEST(TestLogBuffer::TestSearchEmpty);
    RUN_TEST(TestLogBuffer::TestMessageTruncation);
    RUN_TEST(TestLogBuffer::TestTimestamp);
}
