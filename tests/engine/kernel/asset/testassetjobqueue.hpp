/**
 * @file testassetjobqueue.hpp
 * @brief Unit tests for the AssetJobQueue class.
 *
 * @date 02/05/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/kernel/asset/asset_job_queue.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestAssetJobQueue
 * @brief Contains static test methods for the AssetJobQueue class.
 */
class TestAssetJobQueue {
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
