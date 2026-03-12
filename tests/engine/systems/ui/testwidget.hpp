// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testwidget.hpp
 * @brief Unit tests for the ui::Widget system.
 *
 * @date 03/08/2026
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/ui/ui_context.hpp>
#include <utils/testhelpers.hpp>

class TestWidget {
public:
    // String interning
    static void TestStringInternBasic();
    static void TestStringInternDuplicate();
    static void TestStringInternEmpty();

    // Widget pool
    static void TestPoolAllocate();
    static void TestPoolFree();
    static void TestPoolCapacity();
    static void TestPoolFreeReuse();

    // Widget creation via UIContext
    static void TestCreatePanel();
    static void TestCreateLabel();
    static void TestCreateButton();
    static void TestCreateSlider();
    static void TestCreateCheckbox();
    static void TestCreateTextField();
    static void TestCreateSeparator();

    // Widget tree
    static void TestSetParent();
    static void TestDestroyWidget();
    static void TestDestroyWidgetRecursive();
    static void TestReparent();

    // Layout
    static void TestLayoutRootFillsViewport();
    static void TestLayoutColumn();
    static void TestLayoutRow();
    static void TestLayoutPercentSize();
    static void TestLayoutFillRemaining();
    static void TestLayoutPadding();
    static void TestLayoutCenterAlignment();

    // Events
    static void TestHitTest();
    static void TestHitTestNested();
    static void TestClickCallback();
    static void TestSliderDrag();
    static void TestCheckboxToggle();
    static void TestFocusTab();

    // Styling
    static void TestThemeDefaults();
    static void TestThemeResolve();
    static void TestPseudoStateHovered();

    // UIContext
    static void TestSetViewport();
    static void TestFindWidget();
    static void TestSetText();
    static void TestSetVisible();

    // New widget types
    static void TestCreateProgressBar();
    static void TestCreateToggleSwitch();
    static void TestCreateRadioButton();
    static void TestCreateNumberSpinner();
    static void TestToggleSwitchToggle();
    static void TestRadioGroupExclusion();
    static void TestSpinnerClamp();

    static void RunAllTests();
};
