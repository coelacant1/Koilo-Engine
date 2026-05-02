// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

class TestAerodynamics {
public:
    // Atmosphere
    static void TestAtmosphereSeaLevel();
    static void TestAtmospherePressureMonotonic();
    static void TestAtmosphereTropopauseContinuity();
    static void TestAtmosphereDensityAt11km();
    static void TestAtmosphereClampsAltitude();

    // AeroCurve
    static void TestAeroCurveEmptyReturnsZero();
    static void TestAeroCurveSinglePointReturnsValue();
    static void TestAeroCurveMidpointLerp();
    static void TestAeroCurveClampsEndpoints();

    // WindField
    static void TestConstantWindReturnsConstant();
    static void TestShearWindLinearWithAltitude();

    // AerodynamicSurface compute
    static void TestSurfaceZeroAirflowZeroForce();
    static void TestSurfaceLevelFlowZeroAoaDragOnly();
    static void TestSurfacePositiveAoaPositiveLift();
    static void TestSurfacePureSideslipNearZeroForce();
    static void TestSurfaceForceAppliesLeverArmTorque();

    // ThrustEngine
    static void TestThrustZeroThrottleZeroForce();
    static void TestThrustFullThrottleAccelerates();
    static void TestThrustFuelBurnDecreasesMass();
    static void TestThrustFuelExhaustionStopsThrust();

    // World + integration
    static void TestWorldRegisterUnregister();
    static void TestPreSubstepHookFiresPerSubstep();
    static void TestModuleAttachToPhysics();

    // wrap-up
    static void TestPreFixedStepKeyedUnregister();
    static void TestPreFixedStepClearStillWorks();
    static void TestEngineWakesSleepingBody();

    // Determinism
    static void TestSameBinaryReplayBitExact();

    static void RunAllTests();
};
