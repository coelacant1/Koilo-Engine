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

    // Drag-and-drop
    static void TestDragPayloadLifecycle();
    static void TestDragThreshold();
    static void TestDropAcceptReject();
    static void TestDragCancelEscape();
    static void TestTreeDragReparent();

    // Sub-menu & popup stack tests
    static void TestPopupStack();
    static void TestMenuItemShortcutText();
    static void TestSubMenuHoverExpand();

    // Command registry tests
    static void TestCommandRegistryBasic();
    static void TestShortcutDispatch();
    static void TestShortcutRebind();

    // Color conversion tests
    static void TestColorHSVRoundtrip();
    static void TestColorFromHex();

    // Virtual list tests
    static void TestVirtualListCreate();
    static void TestVirtualListScroll();
    static void TestVirtualListRowRecycle();

    // Canvas2D tests
    static void TestCanvas2DCreate();
    static void TestCanvas2DDrawCommands();

    // Settings store tests
    static void TestSettingsGetSet();
    static void TestSettingsSerialize();
    static void TestSettingsDefaults();

    // Icon system tests

    static void TestIconFromName();

    // Color picker tests
    static void TestColorPickerBuild();
    static void TestColorPickerSVInteraction();

    // Curve editor tests
    static void TestCurveEditorEvaluate();
    static void TestCurveEditorAddRemove();

    // Timeline tests
    static void TestTimelineScrub();

    // Content browser tests
    static void TestFsAdapter();
    static void TestBreadcrumb();
    static void TestContentBrowserBuild();

    // Node graph tests
    static void TestNodeGraphBuild();
    static void TestNodeGraphConnect();

    // TreeRow multi-column tests
    static void TestTreeRowColumns();

    // Preferences panel tests
    static void TestPreferencesBuild();

    // Widget factory (#30)
    static void TestWidgetFactoryRegisterAndCreate();
    static void TestWidgetFactoryListTypes();
    static void TestWidgetFactoryUnknownType();

    static void RunAllTests();
};
