/**
 * @file testvulkangpuinfo.hpp
 * @brief Unit tests for the VulkanGPUInfo class.
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
 * @class TestVulkanGPUInfo
 * @brief Contains static test methods for the VulkanGPUInfo class.
 */
class TestVulkanGPUInfo {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
