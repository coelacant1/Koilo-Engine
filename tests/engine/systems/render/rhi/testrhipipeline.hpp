// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testrhipipeline.hpp
 * @brief RHI pipeline and renderer smoke tests.
 *
 * Tests the unified render pipeline, UI renderer interface,
 * and backend factory without requiring a live GPU context.
 *
 * @date 03/29/2026
 * @author Coela Can't
 */
#pragma once
#include <unity.h>

namespace TestRHIPipeline {
    // Factory tests
    void TestCreateBestRenderBackendReturnsSoftware();
    void TestCreateBestSoftwareBackend();

    // UIRHIRenderer (no device)
    void TestUIRHIRendererDefaultState();
    void TestUIRHIRendererShutdownWithoutInit();
    void TestUIRHIRendererIsSoftwareFalse();

    void RunAllTests();
}
