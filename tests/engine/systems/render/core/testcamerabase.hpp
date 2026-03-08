// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testcamerabase.hpp
 * @brief Unit tests for CameraBase class.
 *
 * @date 24/10/2025
 * @author Coela
 */

#pragma once

#include <unity.h>


class TestCameraBase {
public:
    static void TestDefaultConstruction();
    static void TestTransformAccess();
    static void TestCameraLayoutAccess();
    static void TestLookOffsetManagement();
    static void Test2DModeFlag();
    static void TestVirtualInterfaceAccess();
    static void RunAllTests();
};

