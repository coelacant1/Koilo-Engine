// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testrhitypes.hpp
 * @brief RHI type system smoke tests - handles, formats, descriptors.
 */
#pragma once
#include <unity.h>

namespace TestRHITypes {
    void TestHandleNullSentinel();
    void TestHandleEquality();
    void TestFormatBytesPerPixel();
    void TestFormatIsDepth();
    void TestBufferUsageFlags();
    void TestTextureUsageFlags();
    void TestBufferDescDefaults();
    void TestTextureDescDefaults();
    void TestPipelineDescDefaults();
    void TestRenderPassDescDefaults();
    void TestCapabilityFeatureFlags();
    void TestLimitsDefaults();
    void RunAllTests();
}
