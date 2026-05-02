/**
 * @file testthreadpool.hpp
 * @brief Unit tests for the ThreadPool class.
 *
 * @date 02/05/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/kernel/thread_pool.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestThreadPool
 * @brief Contains static test methods for the ThreadPool class.
 */
class TestThreadPool {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestThreadCount();
    static void TestPendingCount();
    static void TestIsRunning();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
