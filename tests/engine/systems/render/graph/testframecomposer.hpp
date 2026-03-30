// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <unity.h>

namespace TestFrameComposer {
    void TestRegisterAndListProviders();
    void TestPhaseOrdering();
    void TestEnableDisableProvider();
    void TestUnregisterProvider();
    void TestBuildAndExecuteCallsProviders();
    void TestDisabledProviderSkipped();
    void TestReplaceExistingProvider();
    void TestExecutionOrderQuery();
    void RunAllTests();
}
