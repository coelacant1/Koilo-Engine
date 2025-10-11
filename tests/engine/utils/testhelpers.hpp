/**
 * @file testhelpers.hpp
 * @brief Common test utilities and macros for PTX Engine unit tests.
 *
 * Provides helper macros, constants, and utilities to simplify writing
 * unit tests with the Unity test framework.
 *
 * @date 09/10/2025
 * @version 1.0
 * @author Coela
 */

#pragma once

#include <unity.h>
#include <ptx/core/math/vector2d.hpp>
#include <ptx/core/math/vector3d.hpp>
#include <ptx/core/color/rgbcolor.hpp>

/**
 * @brief Assert that two Vector2D objects are equal within tolerance.
 * @param expected The expected Vector2D value
 * @param actual The actual Vector2D value
 */
#define TEST_ASSERT_VECTOR2D_EQUAL(expected, actual) \
    do { \
        TEST_ASSERT_FLOAT_WITHIN(0.001f, (expected).X, (actual).X); \
        TEST_ASSERT_FLOAT_WITHIN(0.001f, (expected).Y, (actual).Y); \
    } while(0)

/**
 * @brief Assert that two Vector2D objects are equal within custom tolerance.
 * @param expected The expected Vector2D value
 * @param actual The actual Vector2D value
 * @param tolerance The tolerance for comparison
 */
#define TEST_ASSERT_VECTOR2D_WITHIN(tolerance, expected, actual) \
    do { \
        TEST_ASSERT_FLOAT_WITHIN((tolerance), (expected).X, (actual).X); \
        TEST_ASSERT_FLOAT_WITHIN((tolerance), (expected).Y, (actual).Y); \
    } while(0)

/**
 * @brief Assert that two Vector3D objects are equal within tolerance.
 * @param expected The expected Vector3D value
 * @param actual The actual Vector3D value
 */
#define TEST_ASSERT_VECTOR3D_EQUAL(expected, actual) \
    do { \
        TEST_ASSERT_FLOAT_WITHIN(0.001f, (expected).X, (actual).X); \
        TEST_ASSERT_FLOAT_WITHIN(0.001f, (expected).Y, (actual).Y); \
        TEST_ASSERT_FLOAT_WITHIN(0.001f, (expected).Z, (actual).Z); \
    } while(0)

/**
 * @brief Assert that two Vector3D objects are equal within custom tolerance.
 * @param expected The expected Vector3D value
 * @param actual The actual Vector3D value
 * @param tolerance The tolerance for comparison
 */
#define TEST_ASSERT_VECTOR3D_WITHIN(tolerance, expected, actual) \
    do { \
        TEST_ASSERT_FLOAT_WITHIN((tolerance), (expected).X, (actual).X); \
        TEST_ASSERT_FLOAT_WITHIN((tolerance), (expected).Y, (actual).Y); \
        TEST_ASSERT_FLOAT_WITHIN((tolerance), (expected).Z, (actual).Z); \
    } while(0)

/**
 * @brief Assert that two RGBColor objects are equal.
 * @param expected The expected RGBColor value
 * @param actual The actual RGBColor value
 */
#define TEST_ASSERT_RGB_EQUAL(expected, actual) \
    do { \
        TEST_ASSERT_EQUAL_UINT8((expected).R, (actual).R); \
        TEST_ASSERT_EQUAL_UINT8((expected).G, (actual).G); \
        TEST_ASSERT_EQUAL_UINT8((expected).B, (actual).B); \
    } while(0)

/**
 * @brief Assert that a vector magnitude is approximately 1.0 (unit vector).
 * @param vec The Vector2D to check
 */
#define TEST_ASSERT_VECTOR2D_IS_UNIT(vec) \
    do { \
        float mag = (vec).Magnitude(); \
        TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, mag); \
    } while(0)

/**
 * @brief Assert that a vector magnitude is approximately 1.0 (unit vector).
 * @param vec The Vector3D to check
 */
#define TEST_ASSERT_VECTOR3D_IS_UNIT(vec) \
    do { \
        float mag = (vec).Magnitude(); \
        TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, mag); \
    } while(0)

/**
 * @brief Assert that a float is approximately zero.
 * @param value The float value to check
 */
#define TEST_ASSERT_FLOAT_IS_ZERO(value) \
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, (value))

/**
 * @brief Assert that a float is approximately one.
 * @param value The float value to check
 */
#define TEST_ASSERT_FLOAT_IS_ONE(value) \
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, (value))

/**
 * @namespace TestHelpers
 * @brief Constants and helper functions for tests.
 */
namespace TestHelpers {
    // Common tolerance values
    constexpr float TIGHT_TOLERANCE    = 0.0001f; ///< Very tight tolerance for precise comparisons
    constexpr float DEFAULT_TOLERANCE  = 0.001f;  ///< Default tolerance for most tests
    constexpr float LOOSE_TOLERANCE    = 0.01f;   ///< Loose tolerance for approximate tests
    constexpr float VERY_LOOSE         = 0.1f;    ///< Very loose tolerance for integration tests

    // Common mathematical constants for validation
    constexpr float PI      = 3.14159265358979323846f;
    constexpr float TWO_PI  = 6.28318530717958647692f;
    constexpr float HALF_PI = 1.57079632679489661923f;
    constexpr float SQRT_2  = 1.41421356237309504880f;
    constexpr float SQRT_3  = 1.73205080756887729352f;

    /**
     * @brief Check if two floats are approximately equal.
     * @param a First value
     * @param b Second value
     * @param tolerance Tolerance for comparison
     * @return true if values are within tolerance
     */
    inline bool FloatEquals(float a, float b, float tolerance = DEFAULT_TOLERANCE) {
        return (a >= b - tolerance) && (a <= b + tolerance);
    }

    /**
     * @brief Check if a float is approximately zero.
     * @param value The value to check
     * @param tolerance Tolerance for comparison
     * @return true if value is within tolerance of zero
     */
    inline bool IsZero(float value, float tolerance = DEFAULT_TOLERANCE) {
        return FloatEquals(value, 0.0f, tolerance);
    }

    /**
     * @brief Check if a float is approximately one.
     * @param value The value to check
     * @param tolerance Tolerance for comparison
     * @return true if value is within tolerance of one
     */
    inline bool IsOne(float value, float tolerance = DEFAULT_TOLERANCE) {
        return FloatEquals(value, 1.0f, tolerance);
    }
}
