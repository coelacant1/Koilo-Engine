/**
 * @file testkeyframetrack.hpp
 * @brief Unit tests for the KeyFrameTrack class.
 *
 * TODO: Add detailed description of what this test suite covers.
 *
 * @date 10/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/systems/scene/animation/keyframetrack.hpp>
#include <utils/testhelpers.hpp>

/**
 * @class TestKeyFrameTrack
 * @brief Contains static test methods for the KeyFrameTrack class.
 */
class TestKeyFrameTrack {
public:
    // Constructor & lifecycle tests
    static void TestDefaultConstructor();
    static void TestParameterizedConstructor();

    // Method tests
    static void TestGetCurrentTime();
    static void TestSetCurrentTime();
    static void TestPause();
    static void TestPlay();
    static void TestSetPlaybackSpeed();
    static void TestGetPlaybackSpeed();
    static void TestSetInterpolationMethod();
    static void TestGetInterpolationMethod();
    static void TestSetMin();
    static void TestSetMax();
    // ... add tests for remaining 14 methods

    // Functionality tests

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void TestAddKeyFrame();
    static void TestAddParameter();
    static void TestGetKeyFrameCapacity();
    static void TestGetKeyFrameCount();
    static void TestGetMax();
    static void TestGetMin();
    static void TestGetParameterCapacity();
    static void TestGetParameterCount();
    static void TestGetParameterValue();
    static void TestIsActive();
    static void TestRemoveKeyFrame();
    static void TestReset();
    static void TestSetRange();
    static void TestUpdate();
    static void RunAllTests();
};
