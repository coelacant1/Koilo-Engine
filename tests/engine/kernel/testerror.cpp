// SPDX-License-Identifier: GPL-3.0-or-later
#include "testerror.hpp"
#include <koilo/kernel/engine_error.hpp>
#include <koilo/kernel/message_types.hpp>
#include <cstring>

using namespace koilo;

void TestError::TestErrorMake() {
    auto err = EngineError::Make(
        ErrorSeverity::Fatal, ErrorSystem::Render,
        "GPU_LOST", "Device lost during present",
        "Check GPU drivers");

    TEST_ASSERT_EQUAL(ErrorSeverity::Fatal, err.severity);
    TEST_ASSERT_EQUAL(ErrorSystem::Render, err.system);
    TEST_ASSERT_EQUAL_STRING("GPU_LOST", err.code);
    TEST_ASSERT_EQUAL_STRING("Device lost during present", err.message);
    TEST_ASSERT_EQUAL_STRING("Check GPU drivers", err.suggestion);
}

void TestError::TestSeverityNames() {
    TEST_ASSERT_EQUAL_STRING("DIAG",  ErrorSeverityName(ErrorSeverity::Diagnostic));
    TEST_ASSERT_EQUAL_STRING("RECV",  ErrorSeverityName(ErrorSeverity::Recoverable));
    TEST_ASSERT_EQUAL_STRING("DEGD",  ErrorSeverityName(ErrorSeverity::Degraded));
    TEST_ASSERT_EQUAL_STRING("FATAL", ErrorSeverityName(ErrorSeverity::Fatal));
}

void TestError::TestSystemNames() {
    TEST_ASSERT_EQUAL_STRING("kernel",  ErrorSystemName(ErrorSystem::Kernel));
    TEST_ASSERT_EQUAL_STRING("render",  ErrorSystemName(ErrorSystem::Render));
    TEST_ASSERT_EQUAL_STRING("asset",   ErrorSystemName(ErrorSystem::Asset));
    TEST_ASSERT_EQUAL_STRING("script",  ErrorSystemName(ErrorSystem::Script));
    TEST_ASSERT_EQUAL_STRING("audio",   ErrorSystemName(ErrorSystem::Audio));
    TEST_ASSERT_EQUAL_STRING("physics", ErrorSystemName(ErrorSystem::Physics));
    TEST_ASSERT_EQUAL_STRING("input",   ErrorSystemName(ErrorSystem::Input));
    TEST_ASSERT_EQUAL_STRING("ui",      ErrorSystemName(ErrorSystem::UI));
    TEST_ASSERT_EQUAL_STRING("ecs",     ErrorSystemName(ErrorSystem::ECS));
    TEST_ASSERT_EQUAL_STRING("network", ErrorSystemName(ErrorSystem::Network));
    TEST_ASSERT_EQUAL_STRING("module",  ErrorSystemName(ErrorSystem::Module));
    TEST_ASSERT_EQUAL_STRING("other",   ErrorSystemName(ErrorSystem::Other));
}

void TestError::TestParseSeverity() {
    TEST_ASSERT_EQUAL(ErrorSeverity::Fatal,       ParseErrorSeverity("fatal"));
    TEST_ASSERT_EQUAL(ErrorSeverity::Degraded,    ParseErrorSeverity("degraded"));
    TEST_ASSERT_EQUAL(ErrorSeverity::Recoverable, ParseErrorSeverity("recoverable"));
    TEST_ASSERT_EQUAL(ErrorSeverity::Diagnostic,  ParseErrorSeverity("diagnostic"));
    TEST_ASSERT_EQUAL(ErrorSeverity::Diagnostic,  ParseErrorSeverity("diag"));
    TEST_ASSERT_EQUAL(ErrorSeverity::Recoverable, ParseErrorSeverity("bogus"));
    TEST_ASSERT_EQUAL(ErrorSeverity::Recoverable, ParseErrorSeverity(nullptr));
}

void TestError::TestParseSystem() {
    TEST_ASSERT_EQUAL(ErrorSystem::Render, ParseErrorSystem("render"));
    TEST_ASSERT_EQUAL(ErrorSystem::Script, ParseErrorSystem("script"));
    TEST_ASSERT_EQUAL(ErrorSystem::Other,  ParseErrorSystem("bogus"));
    TEST_ASSERT_EQUAL(ErrorSystem::Other,  ParseErrorSystem(nullptr));
}

void TestError::TestHistoryPushAndRetrieve() {
    ErrorHistory hist(8);

    auto e1 = EngineError::Make(ErrorSeverity::Recoverable, ErrorSystem::Asset,
                                "MISSING_TEX", "Texture not found");
    auto e2 = EngineError::Make(ErrorSeverity::Fatal, ErrorSystem::Render,
                                "SHADER_FAIL", "Shader compilation failed");
    hist.Push(e1);
    hist.Push(e2);

    TEST_ASSERT_EQUAL(2u, hist.Count());

    const auto* r0 = hist.At(0);
    TEST_ASSERT_NOT_NULL(r0);
    TEST_ASSERT_EQUAL_STRING("MISSING_TEX", r0->code);

    const auto* r1 = hist.At(1);
    TEST_ASSERT_NOT_NULL(r1);
    TEST_ASSERT_EQUAL_STRING("SHADER_FAIL", r1->code);

    TEST_ASSERT_NULL(hist.At(2));
}

void TestError::TestHistoryRingOverflow() {
    ErrorHistory hist(4);

    for (int i = 0; i < 6; ++i) {
        char code[16];
        std::snprintf(code, sizeof(code), "ERR_%d", i);
        hist.Push(EngineError::Make(ErrorSeverity::Recoverable, ErrorSystem::Kernel, code, "msg"));
    }

    // Capacity is 4, pushed 6 - oldest 2 evicted
    TEST_ASSERT_EQUAL(4u, hist.Count());

    const auto* oldest = hist.At(0);
    TEST_ASSERT_NOT_NULL(oldest);
    TEST_ASSERT_EQUAL_STRING("ERR_2", oldest->code);

    const auto* newest = hist.At(3);
    TEST_ASSERT_NOT_NULL(newest);
    TEST_ASSERT_EQUAL_STRING("ERR_5", newest->code);
}

void TestError::TestHistoryQuery() {
    ErrorHistory hist(16);

    hist.Push(EngineError::Make(ErrorSeverity::Fatal,       ErrorSystem::Render, "A", "a"));
    hist.Push(EngineError::Make(ErrorSeverity::Recoverable, ErrorSystem::Render, "B", "b"));
    hist.Push(EngineError::Make(ErrorSeverity::Fatal,       ErrorSystem::Asset,  "C", "c"));
    hist.Push(EngineError::Make(ErrorSeverity::Diagnostic,  ErrorSystem::Kernel, "D", "d"));

    // Filter by severity
    ErrorSeverity fatal = ErrorSeverity::Fatal;
    auto fatals = hist.Query(&fatal, nullptr);
    TEST_ASSERT_EQUAL(2u, fatals.size());
    TEST_ASSERT_EQUAL_STRING("A", fatals[0]->code);
    TEST_ASSERT_EQUAL_STRING("C", fatals[1]->code);

    // Filter by system
    ErrorSystem render = ErrorSystem::Render;
    auto renderErrs = hist.Query(nullptr, &render);
    TEST_ASSERT_EQUAL(2u, renderErrs.size());

    // Filter by both
    auto fatalRender = hist.Query(&fatal, &render);
    TEST_ASSERT_EQUAL(1u, fatalRender.size());
    TEST_ASSERT_EQUAL_STRING("A", fatalRender[0]->code);

    // No filter
    auto all = hist.Query();
    TEST_ASSERT_EQUAL(4u, all.size());
}

void TestError::TestHistoryClear() {
    ErrorHistory hist(8);
    hist.Push(EngineError::Make(ErrorSeverity::Recoverable, ErrorSystem::Kernel, "X", "x"));
    TEST_ASSERT_EQUAL(1u, hist.Count());
    hist.Clear();
    TEST_ASSERT_EQUAL(0u, hist.Count());
    TEST_ASSERT_NULL(hist.At(0));
}

void TestError::TestMessageType() {
    TEST_ASSERT_EQUAL(16, MSG_ENGINE_ERROR);
    TEST_ASSERT_EQUAL_STRING("ENGINE_ERROR", MessageTypeName(MSG_ENGINE_ERROR));
}

void TestError::RunAllTests() {
    RUN_TEST(TestErrorMake);
    RUN_TEST(TestSeverityNames);
    RUN_TEST(TestSystemNames);
    RUN_TEST(TestParseSeverity);
    RUN_TEST(TestParseSystem);
    RUN_TEST(TestHistoryPushAndRetrieve);
    RUN_TEST(TestHistoryRingOverflow);
    RUN_TEST(TestHistoryQuery);
    RUN_TEST(TestHistoryClear);
    RUN_TEST(TestMessageType);
}
