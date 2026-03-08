// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testlight.cpp
 * @brief Implementation of Light unit tests.
 */

#include "testlight.hpp"

using namespace koilo;
// ========== Constructor Tests ==========

void TestLight::TestDefaultConstructor() {
    Light light;
    TEST_ASSERT_TRUE(true);
}

void TestLight::TestParameterizedConstructor() {
    Vector3D pos(10.0f, 20.0f, 30.0f);
    Vector3D intensity(1.0f, 0.8f, 0.6f); // RGB intensity
    float falloff = 10.0f;
    float a = 1.0f;
    float b = 0.5f;
    
    Light light(pos, intensity, falloff, a, b);
    
    Vector3D resultPos = light.GetPosition();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, resultPos.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 20.0f, resultPos.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 30.0f, resultPos.Z);
    
    Vector3D resultIntensity = light.GetIntensity();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, resultIntensity.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.8f, resultIntensity.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.6f, resultIntensity.Z);
    
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, light.GetFalloff());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, light.GetCurveA());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.5f, light.GetCurveB());
}

// ========== Method Tests ==========

void TestLight::TestSet() {
    Light light;
    
    Vector3D pos(5.0f, 10.0f, 15.0f);
    Vector3D intensity(0.9f, 0.7f, 0.5f);
    float falloff = 20.0f;
    
    light.Set(pos, intensity, falloff, 2.0f, 1.0f);
    
    Vector3D resultPos = light.GetPosition();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 5.0f, resultPos.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, resultPos.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 15.0f, resultPos.Z);
    
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 20.0f, light.GetFalloff());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 2.0f, light.GetCurveA());
}

void TestLight::TestSetIntensity() {
    Light light;
    
    Vector3D intensity(0.5f, 0.6f, 0.7f);
    light.SetIntensity(intensity);
    
    Vector3D result = light.GetIntensity();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.5f, result.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.6f, result.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.7f, result.Z);
}

void TestLight::TestSetFalloff() {
    Light light;
    
    // Test single-parameter falloff
    light.SetFalloff(15.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 15.0f, light.GetFalloff());
    
    // Test three-parameter falloff (with curve)
    light.SetFalloff(25.0f, 1.5f, 0.75f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 25.0f, light.GetFalloff());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.5f, light.GetCurveA());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.75f, light.GetCurveB());
}

void TestLight::TestMoveTo() {
    Light light;
    
    Vector3D newPos(100.0f, 200.0f, 300.0f);
    light.MoveTo(newPos);
    
    Vector3D pos = light.GetPosition();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, pos.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 200.0f, pos.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 300.0f, pos.Z);
}

void TestLight::TestTranslate() {
    Light light;
    Vector3D startPos(10.0f, 20.0f, 30.0f);
    light.MoveTo(startPos);
    
    Vector3D translation(5.0f, 10.0f, 15.0f);
    light.Translate(translation);
    
    Vector3D finalPos = light.GetPosition();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 15.0f, finalPos.X); // 10 + 5
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 30.0f, finalPos.Y); // 20 + 10
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 45.0f, finalPos.Z); // 30 + 15
}

void TestLight::TestSetCurve() {
    Light light;
    
    light.SetCurve(2.5f, 1.25f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 2.5f, light.GetCurveA());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.25f, light.GetCurveB());
}

void TestLight::TestGetPosition() {
    Vector3D testPos(7.0f, 14.0f, 21.0f);
    Light light(testPos, Vector3D(1, 1, 1), 10.0f, 1.0f, 1.0f);
    
    Vector3D pos = light.GetPosition();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 7.0f, pos.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 14.0f, pos.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 21.0f, pos.Z);
}

void TestLight::TestGetFalloff() {
    Light light(Vector3D(0, 0, 0), Vector3D(1, 1, 1), 50.0f, 1.0f, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 50.0f, light.GetFalloff());
}

void TestLight::TestGetCurveA() {
    Light light(Vector3D(0, 0, 0), Vector3D(1, 1, 1), 10.0f, 3.0f, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.0f, light.GetCurveA());
}

void TestLight::TestGetCurveB() {
    Light light(Vector3D(0, 0, 0), Vector3D(1, 1, 1), 10.0f, 1.0f, 4.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 4.0f, light.GetCurveB());
}

void TestLight::TestGetIntensity() {
    Vector3D intensity(0.3f, 0.6f, 0.9f);
    Light light(Vector3D(0, 0, 0), intensity, 10.0f, 1.0f, 1.0f);
    
    Vector3D result = light.GetIntensity();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.3f, result.X);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.6f, result.Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.9f, result.Z);
}

// ========== Edge Cases ==========

void TestLight::TestEdgeCases() {
    Light light;
    
    // Test zero intensity (black light)
    light.SetIntensity(Vector3D(0.0f, 0.0f, 0.0f));
    Vector3D intensity = light.GetIntensity();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, intensity.X);
    
    // Test very high intensity
    light.SetIntensity(Vector3D(10.0f, 10.0f, 10.0f));
    intensity = light.GetIntensity();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, intensity.X);
    
    // Test zero falloff (infinite range)
    light.SetFalloff(0.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, light.GetFalloff());
    
    // Test negative translation
    light.MoveTo(Vector3D(50.0f, 50.0f, 50.0f));
    light.Translate(Vector3D(-30.0f, -30.0f, -30.0f));
    Vector3D pos = light.GetPosition();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 20.0f, pos.X);
}

void TestLight::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestSet);
    RUN_TEST(TestSetIntensity);
    RUN_TEST(TestSetFalloff);
    RUN_TEST(TestMoveTo);
    RUN_TEST(TestTranslate);
    RUN_TEST(TestSetCurve);
    RUN_TEST(TestGetPosition);
    RUN_TEST(TestGetFalloff);
    RUN_TEST(TestGetCurveA);
    RUN_TEST(TestGetCurveB);
    RUN_TEST(TestEdgeCases);
    RUN_TEST(TestGetIntensity);
}
