// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testerror.hpp
 * @brief Unit tests for structured error taxonomy (EngineError, ErrorHistory).
 *
 * @date 03/30/2026
 * @author Coela
 */
#pragma once

#include <unity.h>
#include <koilo/kernel/engine_error.hpp>
#include <koilo/kernel/message_types.hpp>
#include <utils/testhelpers.hpp>

class TestError {
public:
    static void TestErrorMake();
    static void TestSeverityNames();
    static void TestSystemNames();
    static void TestParseSeverity();
    static void TestParseSystem();
    static void TestHistoryPushAndRetrieve();
    static void TestHistoryRingOverflow();
    static void TestHistoryQuery();
    static void TestHistoryClear();
    static void TestMessageType();

    static void RunAllTests();
};
