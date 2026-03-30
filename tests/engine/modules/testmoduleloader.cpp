// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testmoduleloader.cpp
 * @brief Implementation of ModuleLoader unit tests.
 */

#include "testmoduleloader.hpp"

using namespace koilo;

// ========== Constructor Tests ==========

void TestModuleLoader::TestDefaultConstructor() {
    // TODO: Implement test for default constructor
    ModuleLoader obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestModuleLoader::TestParameterizedConstructor() {
    // TODO: Implement test for parameterized constructor
    TEST_IGNORE_MESSAGE("Stub");
}

// ========== Method Tests ==========

void TestModuleLoader::TestInitializeAll() {
    // TODO: Implement test for InitializeAll()
    ModuleLoader obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestModuleLoader::TestUpdateAll() {
    // TODO: Implement test for UpdateAll()
    ModuleLoader obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestModuleLoader::TestRenderAll() {
    // TODO: Implement test for RenderAll()
    ModuleLoader obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestModuleLoader::TestShutdownAll() {
    // TODO: Implement test for ShutdownAll()
    ModuleLoader obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestModuleLoader::TestHasModule() {
    // TODO: Implement test for HasModule()
    ModuleLoader obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestModuleLoader::TestListModules() {
    // TODO: Implement test for ListModules()
    ModuleLoader obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestModuleLoader::TestGetModule() {
    // TODO: Implement test for GetModule()
    ModuleLoader obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestModuleLoader::TestUnloadModule() {
    // TODO: Implement test for UnloadModule()
    ModuleLoader obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestModuleLoader::TestLoadFromLibrary() {
    // TODO: Implement test for LoadFromLibrary()
    ModuleLoader obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestModuleLoader::TestScanAndLoad() {
    // TODO: Implement test for ScanAndLoad()
    ModuleLoader obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestModuleLoader::TestTryLoad() {
    // TODO: Implement test for TryLoad()
    ModuleLoader obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestModuleLoader::TestSetModuleSearchPath() {
    // TODO: Implement test for SetModuleSearchPath()
    ModuleLoader obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestModuleLoader::TestGetModuleSearchPath() {
    // TODO: Implement test for GetModuleSearchPath()
    ModuleLoader obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestModuleLoader::TestSetLoadMode() {
    // TODO: Implement test for SetLoadMode()
    ModuleLoader obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestModuleLoader::TestGetLoadMode() {
    // TODO: Implement test for GetLoadMode()
    ModuleLoader obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestModuleLoader::TestReloadModule() {
    // TODO: Implement test for ReloadModule()
    ModuleLoader obj;
    TEST_IGNORE_MESSAGE("Stub");
}

void TestModuleLoader::TestCheckAndReload() {
    // TODO: Implement test for CheckAndReload()
    ModuleLoader obj;
    TEST_IGNORE_MESSAGE("Stub");
}

// ========== Edge Cases ==========

void TestModuleLoader::TestEdgeCases() {
    // TODO: Test edge cases (null, boundaries, extreme values)
    TEST_IGNORE_MESSAGE("Stub");
}

// ========== ABI v3 Tests ==========

void TestModuleLoader::TestAbiV3StructLayout() {
    using namespace koilo;

    // ABI version must be 3
    TEST_ASSERT_EQUAL_INT(3, KL_MODULE_ABI_VER);

    // EngineServices must contain the v3 function pointers
    EngineServices svc{};
    svc.api_size = sizeof(EngineServices);

    // Verify v3 fields exist and are null-initialized
    TEST_ASSERT_NULL(svc.register_command);
    TEST_ASSERT_NULL(svc.register_input_listener);
    TEST_ASSERT_NULL(svc.register_component);
    TEST_ASSERT_NULL(svc.register_widget_type);
    TEST_ASSERT_NULL(svc.register_render_pass);

    // Verify reserved_v3 slots exist
    for (int i = 0; i < 4; ++i)
        TEST_ASSERT_NULL(svc.reserved_v3[i]);

    // EngineServices must be larger than a v2-only struct would be
    // v2 had reserved[4] (4 pointers). v3 replaces with 5 fn ptrs + reserved_v3[4] (9 pointers)
    // So v3 struct is 5 pointers larger than v2 was
    size_t ptrDiff = 5 * sizeof(void*);
    TEST_ASSERT_TRUE(sizeof(EngineServices) >= ptrDiff);

    // Descriptor structs must be non-zero size
    TEST_ASSERT_TRUE(sizeof(KoiloCommandDesc) > 0);
    TEST_ASSERT_TRUE(sizeof(KoiloInputListenerDesc) > 0);
    TEST_ASSERT_TRUE(sizeof(KoiloComponentDesc) > 0);
    TEST_ASSERT_TRUE(sizeof(KoiloWidgetTypeDesc) > 0);
    TEST_ASSERT_TRUE(sizeof(KoiloRenderPassDesc) > 0);
}

void TestModuleLoader::TestAbiV3HasApiBackwardCompat() {
    using namespace koilo;

    // Helper: check if a field is within api_size bounds
    #define HAS_FIELD(svc, field) \
        ((svc).api_size >= (uint32_t)(offsetof(EngineServices, field) + sizeof((svc).field)))

    // Full v3 struct should have all v3 fields accessible
    EngineServices fullSvc{};
    fullSvc.api_size = sizeof(EngineServices);

    TEST_ASSERT_TRUE(HAS_FIELD(fullSvc, register_command));
    TEST_ASSERT_TRUE(HAS_FIELD(fullSvc, register_input_listener));
    TEST_ASSERT_TRUE(HAS_FIELD(fullSvc, register_component));
    TEST_ASSERT_TRUE(HAS_FIELD(fullSvc, register_widget_type));
    TEST_ASSERT_TRUE(HAS_FIELD(fullSvc, register_render_pass));

    // A truncated api_size (simulating v2 module's view) should NOT have v3 fields
    EngineServices truncSvc{};
    truncSvc.api_size = offsetof(EngineServices, register_command);

    TEST_ASSERT_FALSE(HAS_FIELD(truncSvc, register_command));
    TEST_ASSERT_FALSE(HAS_FIELD(truncSvc, register_input_listener));
    TEST_ASSERT_FALSE(HAS_FIELD(truncSvc, register_render_pass));

    // v2 fields should still be accessible even with truncated size
    TEST_ASSERT_TRUE(HAS_FIELD(truncSvc, register_global));
    TEST_ASSERT_TRUE(HAS_FIELD(truncSvc, alloc));
    TEST_ASSERT_TRUE(HAS_FIELD(truncSvc, register_class));

    #undef HAS_FIELD
}

void TestModuleLoader::TestAbiV3AdapterNullSafety() {
    using namespace koilo;

    // All adapters must handle null engine gracefully
    KoiloCommandDesc cmdDesc{};
    cmdDesc.name = "test";
    cmdDesc.handler = [](const char**, int) -> int { return 0; };
    TEST_ASSERT_EQUAL_INT(-1, AbiRegisterCommand(nullptr, &cmdDesc));

    // All adapters must handle null descriptor gracefully
    int dummy = 0;
    TEST_ASSERT_EQUAL_INT(-1, AbiRegisterCommand(&dummy, nullptr));
    TEST_ASSERT_EQUAL_INT(-1, AbiRegisterInputListener(&dummy, nullptr));
    TEST_ASSERT_EQUAL_INT(-1, AbiRegisterComponent(&dummy, nullptr));
    TEST_ASSERT_EQUAL_INT(-1, AbiRegisterWidgetType(&dummy, nullptr));
    TEST_ASSERT_EQUAL_INT(-1, AbiRegisterRenderPass(&dummy, nullptr));

    // Component descriptor with zero size must be rejected
    KoiloComponentDesc compDesc{};
    compDesc.name = "bad";
    compDesc.size = 0;
    TEST_ASSERT_EQUAL_INT(-1, AbiRegisterComponent(&dummy, &compDesc));
}

// ========== Test Runner ==========

void TestModuleLoader::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestInitializeAll);
    RUN_TEST(TestUpdateAll);
    RUN_TEST(TestRenderAll);
    RUN_TEST(TestShutdownAll);
    RUN_TEST(TestHasModule);
    RUN_TEST(TestListModules);
    RUN_TEST(TestGetModule);
    RUN_TEST(TestUnloadModule);
    RUN_TEST(TestLoadFromLibrary);
    RUN_TEST(TestScanAndLoad);
    RUN_TEST(TestTryLoad);
    RUN_TEST(TestSetModuleSearchPath);
    RUN_TEST(TestGetModuleSearchPath);
    RUN_TEST(TestSetLoadMode);
    RUN_TEST(TestGetLoadMode);
    RUN_TEST(TestReloadModule);
    RUN_TEST(TestCheckAndReload);
    RUN_TEST(TestEdgeCases);
    RUN_TEST(TestAbiV3StructLayout);
    RUN_TEST(TestAbiV3HasApiBackwardCompat);
    RUN_TEST(TestAbiV3AdapterNullSafety);
}
