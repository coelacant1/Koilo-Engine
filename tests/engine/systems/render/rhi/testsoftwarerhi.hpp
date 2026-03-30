// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testsoftwarerhi.hpp
 * @brief Unit tests for SoftwareRHIDevice.
 */
#pragma once
#include <unity.h>

namespace TestSoftwareRHI {
    void TestInitializeAndShutdown();
    void TestGetNameAndCaps();
    void TestCreateDestroyBuffer();
    void TestUpdateAndMapBuffer();
    void TestCreateDestroyTexture();
    void TestUpdateTexture();
    void TestCreateDestroyShader();
    void TestCreateDestroyPipeline();
    void TestCreateDestroyRenderPass();
    void TestCreateDestroyFramebuffer();
    void TestSwapchainRenderPass();
    void TestOnResizeAllocatesSwapchain();
    void TestBeginRenderPassClearsColor();
    void TestBeginSwapchainRenderPassClears();
    void TestBindStateTracking();
    void TestDrawIsNoOp();
    void TestTriangleRasterWritesPixels();
    void TestTriangleDepthTest();
    void TestLineRasterWritesPixels();
    void TestBlitFullscreenQuad();
    void RunAllTests();
}
