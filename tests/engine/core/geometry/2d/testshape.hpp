// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testshape.hpp
 * @brief Unit tests for Shape base class.
 *
 * @date 24/10/2025
 * @author Coela
 */

#pragma once

#include <unity.h>


class TestShape {
public:
    static void TestShapeConstruction();
    static void TestShapeBounds();
    static void TestShapeCenter();
    static void TestShapeSize();
    static void TestShapeRotation();
    static void TestIsInShape();
    
    static void RunAllTests();
};

