/**
 * @file testkeyframetrack.hpp
 * @brief Unit tests for the KeyFrameTrack class.
 *
 * @date 02/05/2026
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <koilo/systems/scene/animation/keyframetrack.hpp>
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
    static void TestSetRange();
    static void TestGetMin();
    static void TestGetMax();
    static void TestAddParameter();
    static void TestAddKeyFrame();
    static void TestRemoveKeyFrame();
    static void TestGetParameterValue();
    static void TestReset();
    static void TestUpdate();
    static void TestGetParameterCapacity();
    static void TestGetKeyFrameCapacity();
    static void TestGetParameterCount();
    static void TestGetKeyFrameCount();
    static void TestIsActive();

    // Edge case & integration tests
    static void TestEdgeCases();

    /**
     * @brief Runs all test methods.
     */
    static void RunAllTests();
};
