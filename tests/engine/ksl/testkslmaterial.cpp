// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testkslmaterial.cpp
 * @brief Integration tests for KSLMaterial bridge and meta shader composition.
 */

#include "testkslmaterial.hpp"
#include <cstring>
#include <fstream>
#include <vector>

using namespace koilo;
using namespace ksl;

// Shared helpers
static KSLSymbolTable& GetSymbols() {
    static KSLSymbolTable syms;
    static bool init = false;
    if (!init) { syms.RegisterAll(); init = true; }
    return syms;
}

static bool LoadModule(KSLModule& mod, const char* name) {
    std::string path = std::string("build/shaders/") + name + ".kso";
    return mod.LoadKSO(path, GetSymbols());
}

// ========== KSLMaterial Lifecycle ==========

KSLMaterial mat;
    TEST_ASSERT_TRUE(mat.Bind(&mod));
    TEST_ASSERT_TRUE(mat.IsBound());
    TEST_ASSERT_EQUAL_PTR(&mod, mat.GetModule());

    mat.Unbind();
    TEST_ASSERT_FALSE(mat.IsBound());
    TEST_ASSERT_NULL(mat.GetModule());
}

KSLMaterial mat;
    TEST_ASSERT_TRUE(mat.Bind(&mod1));
    TEST_ASSERT_EQUAL_PTR(&mod1, mat.GetModule());

    TEST_ASSERT_TRUE(mat.Bind(&mod2));
    TEST_ASSERT_EQUAL_PTR(&mod2, mat.GetModule());
}

// ========== Shading Through IMaterial ==========

KSLMaterial mat;
    mat.Bind(&mod);
    mat.SetVec3("color", 0.0f, 1.0f, 0.0f); // green

    Vector3D pos(0,0,0), norm(0,1,0), uv(0.5f, 0.5f, 0);
    SurfaceProperties surf(pos, norm, uv);

    Color888 c = mat.GetShader()->Shade(surf, mat);
    TEST_ASSERT_EQUAL_UINT8(0, c.R);
    TEST_ASSERT_EQUAL_UINT8(255, c.G);
    TEST_ASSERT_EQUAL_UINT8(0, c.B);
}

KSLMaterial mat;
    mat.Bind(&mod);

    // Normal (0,1,0) -> mapped to color space: R=(0+1)/2=0.5, G=(1+1)/2=1.0, B=(0+1)/2=0.5
    Vector3D pos(0,0,0), norm(0,1,0), uv(0,0,0);
    SurfaceProperties surf(pos, norm, uv);

    Color888 c = mat.GetShader()->Shade(surf, mat);
    TEST_ASSERT_UINT8_WITHIN(5, 127, c.R); // ~0.5 * 255 = 127
    TEST_ASSERT_UINT8_WITHIN(5, 255, c.G); // ~1.0 * 255 = 255
    TEST_ASSERT_UINT8_WITHIN(5, 127, c.B); // ~0.5 * 255 = 127
}

// ========== Parameter Setting ==========

void TestKSLMaterial::TestSetFloat() {
    KSLModule mod;
    if (!LoadModule(mod, "uniform_color")) { TEST_IGNORE_MESSAGE("KSO not found"); return; }

    KSLMaterial mat;
    mat.Bind(&mod);
    // SetFloat should not crash even if param doesn't exist
    mat.SetFloat("nonexistent", 1.0f);
    TEST_ASSERT_TRUE(mat.IsBound());
}

void TestKSLMaterial::TestSetVec3() {
    KSLModule mod;
    if (!LoadModule(mod, "uniform_color")) { TEST_IGNORE_MESSAGE("KSO not found"); return; }

    KSLMaterial mat;
    mat.Bind(&mod);
    mat.SetVec3("color", 0.0f, 0.0f, 1.0f); // blue

    Vector3D pos(0,0,0), norm(0,1,0), uv(0,0,0);
    SurfaceProperties surf(pos, norm, uv);
    Color888 c = mat.GetShader()->Shade(surf, mat);

    TEST_ASSERT_EQUAL_UINT8(0, c.R);
    TEST_ASSERT_EQUAL_UINT8(0, c.G);
    TEST_ASSERT_EQUAL_UINT8(255, c.B);
}

// ========== KSLModule Direct Tests ==========

TEST_ASSERT_EQUAL_STRING("spiral", mod.Name().c_str());
    TEST_ASSERT_TRUE(mod.HasCPU());
}

auto params = mod.GetParams();
    TEST_ASSERT_TRUE(params.count > 0);
    TEST_ASSERT_NOT_NULL(params.decls);

    // Spiral should have parameters like "width", "bend", etc.
    bool foundWidth = false;
    for (int i = 0; i < params.count; i++) {
        if (std::strcmp(params.decls[i].name, "width") == 0) {
            foundWidth = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(foundWidth);
}

void* inst1 = mod.CreateInstance();
    void* inst2 = mod.CreateInstance();
    TEST_ASSERT_NOT_NULL(inst1);
    TEST_ASSERT_NOT_NULL(inst2);
    TEST_ASSERT_NOT_EQUAL(inst1, inst2);

    // Destroy should not crash
    mod.DestroyInstance(inst1);
    mod.DestroyInstance(inst2);
}

// ========== RunAllTests ==========

void TestKSLMaterial::TestAddLight() {
    // TODO: Implement test for AddLight()
    KSLMaterial obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestKSLMaterial::TestDefaultConstructor() {
    // TODO: Implement test for default constructor
    KSLMaterial obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestKSLMaterial::TestEdgeCases() {
    // TODO: Test edge cases (null, boundaries, extreme values)
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestKSLMaterial::TestIsBound() {
    // TODO: Implement test for IsBound()
    KSLMaterial obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestKSLMaterial::TestLightCount() {
    // TODO: Implement test for LightCount()
    KSLMaterial obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestKSLMaterial::TestParameterizedConstructor() {
    // TODO: Implement test for parameterized constructor
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestKSLMaterial::TestSetCameraPosition() {
    // TODO: Implement test for SetCameraPosition()
    KSLMaterial obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestKSLMaterial::TestSetColor() {
    // TODO: Implement test for SetColor()
    KSLMaterial obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestKSLMaterial::TestSetColorAt() {
    // TODO: Implement test for SetColorAt()
    KSLMaterial obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestKSLMaterial::TestSetDeltaTime() {
    // TODO: Implement test for SetDeltaTime()
    KSLMaterial obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestKSLMaterial::TestSetInt() {
    // TODO: Implement test for SetInt()
    KSLMaterial obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestKSLMaterial::TestSetLightColor() {
    // TODO: Implement test for SetLightColor()
    KSLMaterial obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestKSLMaterial::TestSetLightCurve() {
    // TODO: Implement test for SetLightCurve()
    KSLMaterial obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestKSLMaterial::TestSetLightFalloff() {
    // TODO: Implement test for SetLightFalloff()
    KSLMaterial obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestKSLMaterial::TestSetLightIntensity() {
    // TODO: Implement test for SetLightIntensity()
    KSLMaterial obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestKSLMaterial::TestSetLightPosition() {
    // TODO: Implement test for SetLightPosition()
    KSLMaterial obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestKSLMaterial::TestSetRainbowPalette() {
    // TODO: Implement test for SetRainbowPalette()
    KSLMaterial obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestKSLMaterial::TestSetTexture() {
    // TODO: Implement test for SetTexture()
    KSLMaterial obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestKSLMaterial::TestSetTime() {
    // TODO: Implement test for SetTime()
    KSLMaterial obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestKSLMaterial::TestSetVec2() {
    // TODO: Implement test for SetVec2()
    KSLMaterial obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestKSLMaterial::TestSetVec3Array() {
    // TODO: Implement test for SetVec3Array()
    KSLMaterial obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestKSLMaterial::TestUnbind() {
    // TODO: Implement test for Unbind()
    KSLMaterial obj;
    TEST_ASSERT_TRUE(false);  // Not implemented
}

void TestKSLMaterial::RunAllTests() {
    RUN_TEST(TestKSLMaterial::TestDefaultConstruction);
    RUN_TEST(TestKSLMaterial::TestBindUnbind);
    RUN_TEST(TestKSLMaterial::TestBindNullModule);
    RUN_TEST(TestKSLMaterial::TestDoubleBindRebinds);

    RUN_TEST(TestKSLMaterial::TestShadeUniformColor);
    RUN_TEST(TestKSLMaterial::TestShadeNormalShader);
    RUN_TEST(TestKSLMaterial::TestShadeUnboundReturnsPink);

    RUN_TEST(TestKSLMaterial::TestSetFloat);
    RUN_TEST(TestKSLMaterial::TestSetVec3);

    RUN_TEST(TestKSLMaterial::TestModuleInfo);
    RUN_TEST(TestKSLMaterial::TestModuleParams);
    RUN_TEST(TestKSLMaterial::TestModuleCreateDestroy);
    RUN_TEST(TestAddLight);
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestEdgeCases);
    RUN_TEST(TestIsBound);
    RUN_TEST(TestLightCount);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestSetCameraPosition);
    RUN_TEST(TestSetColor);
    RUN_TEST(TestSetColorAt);
    RUN_TEST(TestSetDeltaTime);
    RUN_TEST(TestSetInt);
    RUN_TEST(TestSetLightColor);
    RUN_TEST(TestSetLightCurve);
    RUN_TEST(TestSetLightFalloff);
    RUN_TEST(TestSetLightIntensity);
    RUN_TEST(TestSetLightPosition);
    RUN_TEST(TestSetRainbowPalette);
    RUN_TEST(TestSetTexture);
    RUN_TEST(TestSetTime);
    RUN_TEST(TestSetVec2);
    RUN_TEST(TestSetVec3Array);
    RUN_TEST(TestUnbind);
}
