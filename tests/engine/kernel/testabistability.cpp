// SPDX-License-Identifier: GPL-3.0-or-later
#include "testabistability.hpp"
#include <koilo/kernel/module_api.hpp>
#include <koilo/kernel/stability.hpp>
#include <koilo/kernel/deprecation.hpp>
#include <cstddef>
#include <cstring>

using namespace koilo;

// -- ABI constant tests --

static void test_ModuleMagicConstant() {
    TEST_ASSERT_EQUAL_HEX32(0x4D494F4B, KL_MODULE_MAGIC);
}

static void test_ModuleAbiVersion() {
    TEST_ASSERT_EQUAL_UINT32(3, KL_MODULE_ABI_VER);
}

// -- KoiloModuleHeader frozen layout --

static void test_ModuleHeaderFieldOrder() {
    KoiloModuleHeader hdr{};
    hdr.magic = 0x4D494F4B;
    hdr.abi_version = 3;
    std::strncpy(hdr.name, "test", sizeof(hdr.name));
    std::strncpy(hdr.version, "1.0", sizeof(hdr.version));
    hdr.phase = 1;
    hdr.flags = 0;

    TEST_ASSERT_EQUAL_HEX32(0x4D494F4B, hdr.magic);
    TEST_ASSERT_EQUAL_UINT32(3, hdr.abi_version);
    TEST_ASSERT_EQUAL_STRING("test", hdr.name);
    TEST_ASSERT_EQUAL_STRING("1.0", hdr.version);
}

static void test_ModuleHeaderFieldOffsets() {
    // Frozen struct: field offsets must never change.
    TEST_ASSERT_EQUAL_UINT32(0, offsetof(KoiloModuleHeader, magic));
    TEST_ASSERT_EQUAL_UINT32(4, offsetof(KoiloModuleHeader, abi_version));
    TEST_ASSERT_EQUAL_UINT32(8, offsetof(KoiloModuleHeader, name));
    TEST_ASSERT_EQUAL_UINT32(40, offsetof(KoiloModuleHeader, version));
    TEST_ASSERT_EQUAL_UINT32(56, offsetof(KoiloModuleHeader, phase));
    TEST_ASSERT_EQUAL_UINT32(60, offsetof(KoiloModuleHeader, flags));
    TEST_ASSERT_EQUAL_UINT32(64, offsetof(KoiloModuleHeader, reserved));
}

// -- EngineServices frozen layout --

static void test_EngineServicesApiSizeFirst() {
    // api_size must always be at offset 0 for forward-compat checks.
    TEST_ASSERT_EQUAL_UINT32(0, offsetof(EngineServices, api_size));
}

static void test_EngineServicesV1FieldsPresent() {
    // Core v1 function pointers must exist at stable offsets.
    EngineServices svc{};
    svc.api_size = sizeof(EngineServices);
    TEST_ASSERT_TRUE(svc.api_size >= offsetof(EngineServices, log_error)
                     + sizeof(svc.log_error));
}

static void test_EngineServicesV3FieldsPresent() {
    // ABI v3 extension points must exist.
    EngineServices svc{};
    svc.api_size = sizeof(EngineServices);
    TEST_ASSERT_TRUE(svc.api_size >= offsetof(EngineServices, reserved_v3)
                     + sizeof(svc.reserved_v3));
}

// -- KoiloFieldKind frozen enum --

static void test_FieldKindValues() {
    // Enum values are frozen and must never change.
    TEST_ASSERT_EQUAL_UINT32(0, KL_KIND_NONE);
    TEST_ASSERT_EQUAL_UINT32(1, KL_KIND_FLOAT);
    TEST_ASSERT_EQUAL_UINT32(2, KL_KIND_INT);
    TEST_ASSERT_EQUAL_UINT32(3, KL_KIND_BOOL);
    TEST_ASSERT_EQUAL_UINT32(4, KL_KIND_STRING);
    TEST_ASSERT_EQUAL_UINT32(5, KL_KIND_VEC3);
    TEST_ASSERT_EQUAL_UINT32(6, KL_KIND_COLOR);
    TEST_ASSERT_EQUAL_UINT32(7, KL_KIND_OBJECT);
    TEST_ASSERT_EQUAL_UINT32(8, KL_KIND_VOID);
}

// -- KoiloMethodExport frozen layout --

static void test_MethodExportFieldOffsets() {
    TEST_ASSERT_EQUAL_UINT32(0, offsetof(KoiloMethodExport, name));
    // invoker follows doc (both pointer-sized)
    TEST_ASSERT_TRUE(offsetof(KoiloMethodExport, invoker)
                     > offsetof(KoiloMethodExport, doc));
    // argc follows invoker
    TEST_ASSERT_TRUE(offsetof(KoiloMethodExport, argc)
                     > offsetof(KoiloMethodExport, invoker));
}

// -- Stability macros compile --

// These just verify the macros expand without error when applied.
KL_FROZEN struct TestFrozenStruct { int x; };
KL_STABLE struct TestStableStruct { int x; };
KL_UNSTABLE struct TestUnstableStruct { int x; };
KL_INTERNAL struct TestInternalStruct { int x; };

static void test_StabilityMacrosCompile() {
    TestFrozenStruct a{1};
    TestStableStruct b{2};
    TestUnstableStruct c{3};
    TestInternalStruct d{4};
    TEST_ASSERT_EQUAL_INT(1, a.x);
    TEST_ASSERT_EQUAL_INT(2, b.x);
    TEST_ASSERT_EQUAL_INT(3, c.x);
    TEST_ASSERT_EQUAL_INT(4, d.x);
}

// -- Deprecation macros compile --

// Verify macro expansion. Actual deprecation warnings are suppressed
// because we are testing the macro itself, not triggering warnings.
#if defined(__GNUC__) || defined(__clang__)
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#elif defined(_MSC_VER)
  #pragma warning(push)
  #pragma warning(disable: 4996)
#endif

KL_DEPRECATED("Use NewFunc instead")
static int DeprecatedFunc() { return 42; }

KL_DEPRECATED_SINCE(0, 4, 0, "Replaced by NewWidget")
static int DeprecatedSinceFunc() { return 99; }

static void test_DeprecationMacrosCompile() {
    TEST_ASSERT_EQUAL_INT(42, DeprecatedFunc());
    TEST_ASSERT_EQUAL_INT(99, DeprecatedSinceFunc());
}

#if defined(__GNUC__) || defined(__clang__)
  #pragma GCC diagnostic pop
#elif defined(_MSC_VER)
  #pragma warning(pop)
#endif

// -- Registration --

void TestAbiStability::RunAllTests() {
    RUN_TEST(test_ModuleMagicConstant);
    RUN_TEST(test_ModuleAbiVersion);
    RUN_TEST(test_ModuleHeaderFieldOrder);
    RUN_TEST(test_ModuleHeaderFieldOffsets);
    RUN_TEST(test_EngineServicesApiSizeFirst);
    RUN_TEST(test_EngineServicesV1FieldsPresent);
    RUN_TEST(test_EngineServicesV3FieldsPresent);
    RUN_TEST(test_FieldKindValues);
    RUN_TEST(test_MethodExportFieldOffsets);
    RUN_TEST(test_StabilityMacrosCompile);
    RUN_TEST(test_DeprecationMacrosCompile);
}
