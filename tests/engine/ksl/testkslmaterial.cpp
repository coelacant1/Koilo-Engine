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

void TestKSLMaterial::TestDefaultConstruction() {
    KSLMaterial mat;
    TEST_ASSERT_FALSE(mat.IsBound());
    TEST_ASSERT_NOT_NULL(mat.GetShader());
}

void TestKSLMaterial::TestBindUnbind() {
    KSLModule mod;
    if (!LoadModule(mod, "uniform_color")) { TEST_IGNORE_MESSAGE("KSO not found"); return; }

    KSLMaterial mat;
    TEST_ASSERT_TRUE(mat.Bind(&mod));
    TEST_ASSERT_TRUE(mat.IsBound());
    TEST_ASSERT_EQUAL_PTR(&mod, mat.GetModule());

    mat.Unbind();
    TEST_ASSERT_FALSE(mat.IsBound());
    TEST_ASSERT_NULL(mat.GetModule());
}

void TestKSLMaterial::TestBindNullModule() {
    KSLMaterial mat;
    TEST_ASSERT_FALSE(mat.Bind(nullptr));
    TEST_ASSERT_FALSE(mat.IsBound());
}

void TestKSLMaterial::TestDoubleBindRebinds() {
    KSLModule mod1, mod2;
    if (!LoadModule(mod1, "uniform_color") || !LoadModule(mod2, "normal")) {
        TEST_IGNORE_MESSAGE("KSO not found"); return;
    }

    KSLMaterial mat;
    TEST_ASSERT_TRUE(mat.Bind(&mod1));
    TEST_ASSERT_EQUAL_PTR(&mod1, mat.GetModule());

    TEST_ASSERT_TRUE(mat.Bind(&mod2));
    TEST_ASSERT_EQUAL_PTR(&mod2, mat.GetModule());
}

// ========== Shading Through IMaterial ==========

void TestKSLMaterial::TestShadeUniformColor() {
    KSLModule mod;
    if (!LoadModule(mod, "uniform_color")) { TEST_IGNORE_MESSAGE("KSO not found"); return; }

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

void TestKSLMaterial::TestShadeNormalShader() {
    KSLModule mod;
    if (!LoadModule(mod, "normal")) { TEST_IGNORE_MESSAGE("KSO not found"); return; }

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

void TestKSLMaterial::TestShadeUnboundReturnsPink() {
    KSLMaterial mat;

    Vector3D pos(0,0,0), norm(0,1,0), uv(0,0,0);
    SurfaceProperties surf(pos, norm, uv);

    Color888 c = mat.GetShader()->Shade(surf, mat);
    // Should return pink error (255, 0, 200)
    TEST_ASSERT_EQUAL_UINT8(255, c.R);
    TEST_ASSERT_EQUAL_UINT8(0, c.G);
    TEST_ASSERT_EQUAL_UINT8(200, c.B);
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

void TestKSLMaterial::TestModuleInfo() {
    KSLModule mod;
    if (!LoadModule(mod, "spiral")) { TEST_IGNORE_MESSAGE("KSO not found"); return; }

    TEST_ASSERT_EQUAL_STRING("spiral", mod.Name().c_str());
    TEST_ASSERT_TRUE(mod.HasCPU());
}

void TestKSLMaterial::TestModuleParams() {
    KSLModule mod;
    if (!LoadModule(mod, "spiral")) { TEST_IGNORE_MESSAGE("KSO not found"); return; }

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

void TestKSLMaterial::TestModuleCreateDestroy() {
    KSLModule mod;
    if (!LoadModule(mod, "uniform_color")) { TEST_IGNORE_MESSAGE("KSO not found"); return; }

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
}
