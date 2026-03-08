// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testelfloader.cpp
 * @brief Implementation of KSL ELF loader unit tests.
 */

#include "testelfloader.hpp"
#include <koilo/ksl/ksl_shader.hpp>
#include <cstring>
#include <fstream>
#include <vector>

using namespace ksl;

// Helper: read a .kso file from build/shaders/
static std::vector<uint8_t> ReadKSO(const char* name) {
    std::string path = std::string("build/shaders/") + name;
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f.is_open()) return {};
    auto sz = f.tellg();
    if (sz <= 0) return {};
    f.seekg(0);
    std::vector<uint8_t> data(static_cast<size_t>(sz));
    f.read(reinterpret_cast<char*>(data.data()), sz);
    return data;
}

// ========== ELF Validation Tests ==========

void TestElfLoader::TestRejectEmptyData() {
    KSLElfLoader loader;
    KSLSymbolTable syms;
    TEST_ASSERT_FALSE(loader.Load(nullptr, 0, syms));
    TEST_ASSERT_FALSE(loader.IsLoaded());
}

void TestElfLoader::TestRejectBadMagic() {
    KSLElfLoader loader;
    KSLSymbolTable syms;
    uint8_t bad[64] = {};
    bad[0] = 'N'; bad[1] = 'O'; bad[2] = 'P'; bad[3] = 'E';
    TEST_ASSERT_FALSE(loader.Load(bad, sizeof(bad), syms));
    const char* err = loader.GetError();
    TEST_ASSERT_NOT_NULL(err);
    TEST_ASSERT_TRUE(std::strstr(err, "magic") != nullptr);
}

void TestElfLoader::TestRejectBigEndian() {
    KSLElfLoader loader;
    KSLSymbolTable syms;
    uint8_t data[64] = {};
    data[0] = 0x7f; data[1] = 'E'; data[2] = 'L'; data[3] = 'F';
    data[4] = 1; // ELFCLASS32
    data[5] = 2; // ELFDATA2MSB (big-endian)
    TEST_ASSERT_FALSE(loader.Load(data, sizeof(data), syms));
    TEST_ASSERT_TRUE(std::strstr(loader.GetError(), "endian") != nullptr);
}

void TestElfLoader::TestRejectTruncatedHeader() {
    KSLElfLoader loader;
    KSLSymbolTable syms;
    uint8_t tiny[8] = {0x7f, 'E', 'L', 'F', 2, 1, 1, 0};
    TEST_ASSERT_FALSE(loader.Load(tiny, sizeof(tiny), syms));
}

void TestElfLoader::TestRejectNonSharedObject() {
    // Build a minimal ELF64 header with e_type=ET_EXEC (2) instead of ET_DYN (3)
    KSLElfLoader loader;
    KSLSymbolTable syms;

    // We need at least sizeof(Elf64_Ehdr) = 64 bytes
    uint8_t data[128] = {};
    data[0] = 0x7f; data[1] = 'E'; data[2] = 'L'; data[3] = 'F';
    data[4] = 2; // ELFCLASS64
    data[5] = 1; // ELFDATA2LSB
    // Set e_type = ET_EXEC (2) at offset 16
    data[16] = 2; data[17] = 0;
    TEST_ASSERT_FALSE(loader.Load(data, sizeof(data), syms));
    TEST_ASSERT_TRUE(std::strstr(loader.GetError(), "shared") != nullptr);
}

void TestElfLoader::TestInitialState() {
    KSLElfLoader loader;
    TEST_ASSERT_FALSE(loader.IsLoaded());
    TEST_ASSERT_NULL(loader.FindExport("anything"));
}

void TestElfLoader::TestMoveConstruct() {
    KSLSymbolTable syms;
    syms.RegisterAll();
    auto data = ReadKSO("uniform_color.kso");
    if (data.empty()) { TEST_IGNORE_MESSAGE("uniform_color.kso not found"); return; }

    KSLElfLoader loader1;
    TEST_ASSERT_TRUE(loader1.Load(data.data(), data.size(), syms));
    TEST_ASSERT_TRUE(loader1.IsLoaded());

    KSLElfLoader loader2(std::move(loader1));
    TEST_ASSERT_TRUE(loader2.IsLoaded());
    TEST_ASSERT_FALSE(loader1.IsLoaded());
    TEST_ASSERT_NOT_NULL(loader2.FindExport("ksl_shade"));
}

void TestElfLoader::TestMoveAssign() {
    KSLSymbolTable syms;
    syms.RegisterAll();
    auto data = ReadKSO("uniform_color.kso");
    if (data.empty()) { TEST_IGNORE_MESSAGE("uniform_color.kso not found"); return; }

    KSLElfLoader loader1;
    TEST_ASSERT_TRUE(loader1.Load(data.data(), data.size(), syms));

    KSLElfLoader loader2;
    loader2 = std::move(loader1);
    TEST_ASSERT_TRUE(loader2.IsLoaded());
    TEST_ASSERT_FALSE(loader1.IsLoaded());
}

void TestElfLoader::TestUnload() {
    KSLSymbolTable syms;
    syms.RegisterAll();
    auto data = ReadKSO("uniform_color.kso");
    if (data.empty()) { TEST_IGNORE_MESSAGE("uniform_color.kso not found"); return; }

    KSLElfLoader loader;
    TEST_ASSERT_TRUE(loader.Load(data.data(), data.size(), syms));
    TEST_ASSERT_TRUE(loader.IsLoaded());
    loader.Unload();
    TEST_ASSERT_FALSE(loader.IsLoaded());
    TEST_ASSERT_NULL(loader.FindExport("ksl_shade"));
}

// ========== Symbol Table Tests ==========

void TestElfLoader::TestSymbolTableRegisterAndResolve() {
    KSLSymbolTable syms;
    int dummy = 42;
    syms.Register("my_sym", reinterpret_cast<void*>(&dummy));
    TEST_ASSERT_EQUAL_PTR(&dummy, syms.Resolve("my_sym"));
}

void TestElfLoader::TestSymbolTableResolveUnknown() {
    KSLSymbolTable syms;
    TEST_ASSERT_NULL(syms.Resolve("nonexistent"));
}

void TestElfLoader::TestSymbolTableMathFunctions() {
    KSLSymbolTable syms;
    syms.RegisterAll();
    // Verify key math symbols are registered
    TEST_ASSERT_NOT_NULL(syms.Resolve("sinf"));
    TEST_ASSERT_NOT_NULL(syms.Resolve("cosf"));
    TEST_ASSERT_NOT_NULL(syms.Resolve("sqrtf"));
    TEST_ASSERT_NOT_NULL(syms.Resolve("atan2f"));
    TEST_ASSERT_NOT_NULL(syms.Resolve("powf"));
    // Runtime symbols
    TEST_ASSERT_NOT_NULL(syms.Resolve("malloc"));
    TEST_ASSERT_NOT_NULL(syms.Resolve("free"));
    TEST_ASSERT_NOT_NULL(syms.Resolve("memcpy"));
    TEST_ASSERT_NOT_NULL(syms.Resolve("strcmp"));
}

// ========== Live ELF64 Loading Tests ==========

void TestElfLoader::TestLoadKSO() {
    KSLSymbolTable syms;
    syms.RegisterAll();
    auto data = ReadKSO("uniform_color.kso");
    if (data.empty()) { TEST_IGNORE_MESSAGE("uniform_color.kso not found"); return; }

    KSLElfLoader loader;
    TEST_ASSERT_TRUE(loader.Load(data.data(), data.size(), syms));
    TEST_ASSERT_TRUE(loader.IsLoaded());
}

void TestElfLoader::TestFindExport() {
    KSLSymbolTable syms;
    syms.RegisterAll();
    auto data = ReadKSO("uniform_color.kso");
    if (data.empty()) { TEST_IGNORE_MESSAGE("uniform_color.kso not found"); return; }

    KSLElfLoader loader;
    TEST_ASSERT_TRUE(loader.Load(data.data(), data.size(), syms));

    TEST_ASSERT_NOT_NULL(loader.FindExport("ksl_info"));
    TEST_ASSERT_NOT_NULL(loader.FindExport("ksl_create"));
    TEST_ASSERT_NOT_NULL(loader.FindExport("ksl_destroy"));
    TEST_ASSERT_NOT_NULL(loader.FindExport("ksl_shade"));
    TEST_ASSERT_NOT_NULL(loader.FindExport("ksl_set_param"));
    TEST_ASSERT_NOT_NULL(loader.FindExport("ksl_params"));
    // Non-existent symbol
    TEST_ASSERT_NULL(loader.FindExport("does_not_exist"));
}

void TestElfLoader::TestGetSymbolTyped() {
    KSLSymbolTable syms;
    syms.RegisterAll();
    auto data = ReadKSO("uniform_color.kso");
    if (data.empty()) { TEST_IGNORE_MESSAGE("uniform_color.kso not found"); return; }

    KSLElfLoader loader;
    TEST_ASSERT_TRUE(loader.Load(data.data(), data.size(), syms));

    auto infoFn = loader.GetSymbol<KSLInfoFn>("ksl_info");
    TEST_ASSERT_NOT_NULL(infoFn);

    const KSLShaderInfo* info = infoFn();
    TEST_ASSERT_NOT_NULL(info);
    TEST_ASSERT_EQUAL_STRING("uniform_color", info->name);
}

void TestElfLoader::TestCallShadeFn() {
    KSLSymbolTable syms;
    syms.RegisterAll();
    auto data = ReadKSO("uniform_color.kso");
    if (data.empty()) { TEST_IGNORE_MESSAGE("uniform_color.kso not found"); return; }

    KSLElfLoader loader;
    TEST_ASSERT_TRUE(loader.Load(data.data(), data.size(), syms));

    auto createFn  = loader.GetSymbol<KSLCreateFn>("ksl_create");
    auto destroyFn = loader.GetSymbol<KSLDestroyFn>("ksl_destroy");
    auto shadeFn   = loader.GetSymbol<KSLShadeFn>("ksl_shade");
    auto setParam  = loader.GetSymbol<KSLSetParamFn>("ksl_set_param");
    TEST_ASSERT_NOT_NULL(createFn);
    TEST_ASSERT_NOT_NULL(destroyFn);
    TEST_ASSERT_NOT_NULL(shadeFn);

    void* inst = createFn();
    TEST_ASSERT_NOT_NULL(inst);

    // Set red color
    vec3 red{1.0f, 0.0f, 0.0f};
    if (setParam) setParam(inst, "color", &red, static_cast<int>(ParamType::Vec3), 1);

    ShadeInput input{};
    input.uv = {0.5f, 0.5f};
    vec4 result = shadeFn(inst, &input);

    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, result.x);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result.y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, result.z);

    destroyFn(inst);
}

void TestElfLoader::TestMultipleLoads() {
    KSLSymbolTable syms;
    syms.RegisterAll();

    auto data1 = ReadKSO("uniform_color.kso");
    auto data2 = ReadKSO("normal.kso");
    if (data1.empty() || data2.empty()) {
        TEST_IGNORE_MESSAGE(".kso files not found"); return;
    }

    KSLElfLoader loader1, loader2;
    TEST_ASSERT_TRUE(loader1.Load(data1.data(), data1.size(), syms));
    TEST_ASSERT_TRUE(loader2.Load(data2.data(), data2.size(), syms));

    auto info1 = loader1.GetSymbol<KSLInfoFn>("ksl_info")();
    auto info2 = loader2.GetSymbol<KSLInfoFn>("ksl_info")();
    TEST_ASSERT_EQUAL_STRING("uniform_color", info1->name);
    TEST_ASSERT_EQUAL_STRING("normal", info2->name);
}

void TestElfLoader::TestReloadAfterUnload() {
    KSLSymbolTable syms;
    syms.RegisterAll();
    auto data = ReadKSO("spiral.kso");
    if (data.empty()) { TEST_IGNORE_MESSAGE("spiral.kso not found"); return; }

    KSLElfLoader loader;
    TEST_ASSERT_TRUE(loader.Load(data.data(), data.size(), syms));
    TEST_ASSERT_TRUE(loader.IsLoaded());
    loader.Unload();
    TEST_ASSERT_FALSE(loader.IsLoaded());

    // Reload same data
    TEST_ASSERT_TRUE(loader.Load(data.data(), data.size(), syms));
    TEST_ASSERT_TRUE(loader.IsLoaded());

    auto info = loader.GetSymbol<KSLInfoFn>("ksl_info")();
    TEST_ASSERT_EQUAL_STRING("spiral", info->name);
}

// ========== RunAllTests ==========

void TestElfLoader::RunAllTests() {
    RUN_TEST(TestElfLoader::TestRejectEmptyData);
    RUN_TEST(TestElfLoader::TestRejectBadMagic);
    RUN_TEST(TestElfLoader::TestRejectBigEndian);
    RUN_TEST(TestElfLoader::TestRejectTruncatedHeader);
    RUN_TEST(TestElfLoader::TestRejectNonSharedObject);
    RUN_TEST(TestElfLoader::TestInitialState);
    RUN_TEST(TestElfLoader::TestMoveConstruct);
    RUN_TEST(TestElfLoader::TestMoveAssign);
    RUN_TEST(TestElfLoader::TestUnload);

    RUN_TEST(TestElfLoader::TestSymbolTableRegisterAndResolve);
    RUN_TEST(TestElfLoader::TestSymbolTableResolveUnknown);
    RUN_TEST(TestElfLoader::TestSymbolTableMathFunctions);

    RUN_TEST(TestElfLoader::TestLoadKSO);
    RUN_TEST(TestElfLoader::TestFindExport);
    RUN_TEST(TestElfLoader::TestGetSymbolTyped);
    RUN_TEST(TestElfLoader::TestCallShadeFn);
    RUN_TEST(TestElfLoader::TestMultipleLoads);
    RUN_TEST(TestElfLoader::TestReloadAfterUnload);
}
