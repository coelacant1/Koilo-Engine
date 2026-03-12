// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testanimation.hpp
 * @brief Test declarations for UI animation system.
 * @date 03/09/2026
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/ui/ui_animation.hpp>

class TestAnimation {
public:
    static void TestEaseLinear();
    static void TestEaseInQuad();
    static void TestEaseOutBounce();
    static void TestTweenStartCancel();
    static void TestTweenUpdate();
    static void TestTweenDelay();
    static void TestTweenLoop();
    static void TestTweenPingPong();
    static void TestTweenPoolFull();
    static void TestTweenOnComplete();
    static void TestCancelAll();

    static void RunAllTests();
};
