/**
 * @file testeditormodule.hpp
 * @brief Unit tests for the EditorModule class.
 *
 * @date 02/05/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/editor/editor_module.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestEditorModule
 * @brief Contains static test methods for the EditorModule class.
 */
class TestEditorModule {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestGetInfo();
    static void TestUpdate();
    static void TestShutdown();
    static void TestIsActive();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
