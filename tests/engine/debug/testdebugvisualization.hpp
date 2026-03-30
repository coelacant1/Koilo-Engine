// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <unity.h>

namespace TestDebugVisualization {
    void TestDrawWireBoxEmitsLines();
    void TestDrawWireSphereEmitsLines();
    void TestDrawSceneBoundsNullSafe();
    void TestDrawPhysicsCollidersNullSafe();
    void TestDrawCameraFrustumNullSafe();
    void TestDrawCrossEmitsSixVertices();
    void TestCVarDefaultsOff();
    void RunAllTests();
}
