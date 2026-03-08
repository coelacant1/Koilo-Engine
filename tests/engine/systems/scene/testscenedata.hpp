// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testscenedata.hpp
 * @brief Unit tests for the SceneData structures.
 *
 * Tests the compiled scene data structures for:
 * - Proper size and alignment
 * - Serialization consistency
 * - Data integrity
 * - Cross-platform compatibility
 *
 * @date 12/21/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/scene/scenedata.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestSceneData
 * @brief Contains static test methods for SceneData structures.
 */
class TestSceneData {
public:
    // Structure size tests
    static void TestStructureSizes();
    static void TestHeaderSize();
    static void TestAlignment();
    
    // Data initialization tests
    static void TestVector3DData();
    static void TestQuaternionData();
    static void TestTransformData();
    static void TestIndexGroupData();
    static void TestTriangleGroupData();
    
    // Material data tests
    static void TestMaterialData_UniformColor();
    static void TestMaterialData_PhongLight();
    static void TestMaterialData_Union();
    
    // Display backend tests
    static void TestDisplayBackendData_HUB75();
    static void TestDisplayBackendData_SPI();
    static void TestDisplayBackendData_Union();
    
    // Scene header tests
    static void TestCompiledSceneHeader_Magic();
    static void TestCompiledSceneHeader_Offsets();
    static void TestCompiledSceneHeader_Sizes();
    
    // Constants tests
    static void TestConstants();
    
    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
