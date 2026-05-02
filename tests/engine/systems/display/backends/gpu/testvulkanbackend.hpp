/**
 * @file testvulkanbackend.hpp
 * @brief Unit tests for the VulkanBackend class.
 *
 * @date 02/05/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/display/backends/gpu/vulkanbackend.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestVulkanBackend
 * @brief Contains static test methods for the VulkanBackend class.
 */
class TestVulkanBackend {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestShutdown();
    static void TestIsInitialized();
    static void TestGetInfo();
    static void TestHasCapability();
    static void TestPresent();
    static void TestWaitVSync();
    static void TestClear();
    static void TestSetRefreshRate();
    static void TestSetOrientation();
    static void TestSetBrightness();
    static void TestSetVSyncEnabled();
    static void TestPresentNoSwap();
    static void TestSetNearestFiltering();
    static void TestPrepareDefaultFramebuffer();
    static void TestGetSelectedGPUIndex();
    static void TestGetInstance();
    static void TestGetPhysicalDevice();
    static void TestGetDevice();
    static void TestGetGraphicsQueue();
    static void TestGetGraphicsFamily();
    static void TestGetRenderPass();
    static void TestGetSwapchainWidth();
    static void TestGetSwapchainHeight();
    static void TestGetSwapchainFormat();
    static void TestGetCurrentFrame();
    static void TestBeginFrame();
    static void TestBeginSwapchainRenderPass();
    static void TestSetTitle();
    static void TestSetFullscreen();
    static void TestIsWindowOpen();
    static void TestGetWindowSize();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
