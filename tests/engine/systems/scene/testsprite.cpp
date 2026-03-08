// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testsprite.cpp
 * @brief Implementation of Sprite unit tests.
 */

#include "testsprite.hpp"

using namespace koilo;

// ========== Constructor Tests ==========

void TestSprite::TestDefaultConstructor() {
    Sprite obj;
    // Uninitialized sprite has no mesh
    TEST_ASSERT_TRUE(obj.GetMesh() == nullptr);
    TEST_ASSERT_TRUE(obj.GetTransform() == nullptr);
    TEST_ASSERT_TRUE(obj.GetMaterial() == nullptr);
}

void TestSprite::TestParameterizedConstructor() {
    // Texture* can be nullptr for unit testing (material is created but texture is null)
    Sprite obj(nullptr, 32.0f, 32.0f);
    TEST_ASSERT_NOT_NULL(obj.GetMesh());
    TEST_ASSERT_NOT_NULL(obj.GetTransform());
    TEST_ASSERT_NOT_NULL(obj.GetMaterial());
}

// ========== Method Tests ==========

void TestSprite::TestGetMesh() {
    Sprite obj;
    TEST_ASSERT_TRUE(obj.GetMesh() == nullptr);

    obj.Init(nullptr, 16.0f, 16.0f);
    TEST_ASSERT_NOT_NULL(obj.GetMesh());
}

void TestSprite::TestGetTransform() {
    Sprite obj(nullptr, 10.0f, 10.0f);
    Transform* t = obj.GetTransform();
    TEST_ASSERT_NOT_NULL(t);
}

void TestSprite::TestSetFrame() {
    Sprite obj(nullptr, 64.0f, 64.0f);
    // Should not crash; verifies material interaction
    obj.SetFrame(0, 0, 32, 32);
    TEST_ASSERT_NOT_NULL(obj.GetMaterial());
}

void TestSprite::TestSetFrameIndex() {
    Sprite obj(nullptr, 64.0f, 64.0f);
    // Should not crash
    obj.SetFrameIndex(0, 32, 32, 4);
    TEST_ASSERT_NOT_NULL(obj.GetMaterial());
}

void TestSprite::TestSetHueAngle() {
    Sprite obj(nullptr, 32.0f, 32.0f);
    obj.SetHueAngle(90.0f);
    TEST_ASSERT_NOT_NULL(obj.GetMaterial());
}

void TestSprite::TestSetEnabled() {
    Sprite obj(nullptr, 32.0f, 32.0f);
    TEST_ASSERT_TRUE(obj.IsEnabled());

    obj.SetEnabled(false);
    TEST_ASSERT_FALSE(obj.IsEnabled());

    obj.SetEnabled(true);
    TEST_ASSERT_TRUE(obj.IsEnabled());
}

void TestSprite::TestIsEnabled() {
    Sprite obj(nullptr, 32.0f, 32.0f);
    TEST_ASSERT_TRUE(obj.IsEnabled());

    obj.SetEnabled(false);
    TEST_ASSERT_FALSE(obj.IsEnabled());
}

void TestSprite::TestGetMaterial() {
    Sprite uninit;
    TEST_ASSERT_TRUE(uninit.GetMaterial() == nullptr);

    Sprite obj(nullptr, 32.0f, 32.0f);
    TEST_ASSERT_NOT_NULL(obj.GetMaterial());
}

// ========== Edge Cases ==========

void TestSprite::TestEdgeCases() {
    // Uninitialized sprite operations should not crash
    Sprite obj;
    obj.SetEnabled(true);
    obj.SetFrame(0, 0, 16, 16);
    obj.SetHueAngle(45.0f);
    TEST_ASSERT_FALSE(obj.IsEnabled());

    // Init after default construction
    obj.Init(nullptr, 8.0f, 8.0f);
    TEST_ASSERT_NOT_NULL(obj.GetMesh());
    TEST_ASSERT_TRUE(obj.IsEnabled());
}

// ========== Test Runner ==========

void TestSprite::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestGetMesh);
    RUN_TEST(TestGetTransform);
    RUN_TEST(TestSetFrame);
    RUN_TEST(TestSetFrameIndex);
    RUN_TEST(TestSetHueAngle);
    RUN_TEST(TestSetEnabled);
    RUN_TEST(TestIsEnabled);
    RUN_TEST(TestGetMaterial);
    RUN_TEST(TestEdgeCases);
}
