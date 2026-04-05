// SPDX-License-Identifier: GPL-3.0-or-later
#include "testschemaversion.hpp"
#include <koilo/kernel/schema_version.hpp>
#include <koilo/kernel/schema_migrator.hpp>
#include <fstream>
#include <cstdio>
#include <cstring>

using namespace koilo;

// -- Binary header tests --

static void test_WriteAndReadSchemaHeader() {
    const char* path = "/tmp/koilo_test_schema.bin";
    {
        std::ofstream f(path, std::ios::binary);
        TEST_ASSERT_TRUE(f.is_open());
        char magic[4] = {'T','S','T','1'};
        WriteSchemaHeader(f, magic, 42);
    }
    {
        std::ifstream f(path, std::ios::binary);
        char expected[4] = {'T','S','T','1'};
        auto hdr = ReadSchemaHeader(f, expected);
        TEST_ASSERT_TRUE(hdr.valid);
        TEST_ASSERT_EQUAL_UINT32(42, hdr.version);
        TEST_ASSERT_EQUAL_MEMORY(expected, hdr.magic, 4);
    }
    std::remove(path);
}

static void test_ReadSchemaHeader_WrongMagic() {
    const char* path = "/tmp/koilo_test_schema_bad.bin";
    {
        std::ofstream f(path, std::ios::binary);
        char magic[4] = {'A','B','C','D'};
        WriteSchemaHeader(f, magic, 1);
    }
    {
        std::ifstream f(path, std::ios::binary);
        char expected[4] = {'X','Y','Z','W'};
        auto hdr = ReadSchemaHeader(f, expected);
        TEST_ASSERT_FALSE(hdr.valid);
    }
    std::remove(path);
}

// -- Text version tests --

static void test_ParseTextSchemaVersion_Valid() {
    uint32_t v = ParseTextSchemaVersion("# koilo config v3", "config");
    TEST_ASSERT_EQUAL_UINT32(3, v);
}

static void test_ParseTextSchemaVersion_WrongFormat() {
    uint32_t v = ParseTextSchemaVersion("# koilo config v3", "level");
    TEST_ASSERT_EQUAL_UINT32(0, v);
}

static void test_ParseTextSchemaVersion_NoHeader() {
    uint32_t v = ParseTextSchemaVersion("key = value", "config");
    TEST_ASSERT_EQUAL_UINT32(0, v);
}

static void test_MakeTextSchemaHeader() {
    std::string hdr = MakeTextSchemaHeader("config", 5);
    TEST_ASSERT_EQUAL_STRING("# koilo config v5", hdr.c_str());
}

// -- SchemaMigrator tests --

static void test_Migrator_ChainSteps() {
    SchemaMigrator migrator;
    migrator.Register("test", 1, 2, [](MigrationContext& ctx) {
        ctx.Rename("old_key", "new_key");
    });
    migrator.Register("test", 2, 3, [](MigrationContext& ctx) {
        ctx.SetDefault("added_in_v3", "default_val");
    });

    MigrationContext::KVMap data = {{"old_key", "hello"}};
    auto result = migrator.Migrate("test", 1, 3, data);
    TEST_ASSERT_TRUE(result.success);
    TEST_ASSERT_EQUAL_INT(2, result.stepsApplied);
    TEST_ASSERT_EQUAL_UINT32(3, result.toVersion);
    // old_key renamed to new_key
    TEST_ASSERT_TRUE(data.find("old_key") == data.end());
    TEST_ASSERT_EQUAL_STRING("hello", data["new_key"].c_str());
    // new default added
    TEST_ASSERT_EQUAL_STRING("default_val", data["added_in_v3"].c_str());
}

static void test_Migrator_SameVersion() {
    SchemaMigrator migrator;
    MigrationContext::KVMap data;
    auto result = migrator.Migrate("any", 5, 5, data);
    TEST_ASSERT_TRUE(result.success);
    TEST_ASSERT_EQUAL_INT(0, result.stepsApplied);
}

static void test_Migrator_DowngradeRejected() {
    SchemaMigrator migrator;
    MigrationContext::KVMap data;
    auto result = migrator.Migrate("any", 3, 1, data);
    TEST_ASSERT_FALSE(result.success);
    TEST_ASSERT_TRUE(result.error.find("Downgrade") != std::string::npos);
}

static void test_Migrator_MissingStep() {
    SchemaMigrator migrator;
    migrator.Register("test", 1, 2, [](MigrationContext&) {});
    // Gap: no v2->v3
    MigrationContext::KVMap data;
    auto result = migrator.Migrate("test", 1, 3, data);
    TEST_ASSERT_FALSE(result.success);
    TEST_ASSERT_EQUAL_UINT32(2, result.toVersion);
}

static void test_Migrator_CanMigrate() {
    SchemaMigrator migrator;
    migrator.Register("fmt", 1, 2, [](MigrationContext&) {});
    migrator.Register("fmt", 2, 3, [](MigrationContext&) {});
    TEST_ASSERT_TRUE(migrator.CanMigrate("fmt", 1, 3));
    TEST_ASSERT_TRUE(migrator.CanMigrate("fmt", 2, 3));
    TEST_ASSERT_TRUE(migrator.CanMigrate("fmt", 3, 3));
    TEST_ASSERT_FALSE(migrator.CanMigrate("fmt", 1, 4));
    TEST_ASSERT_FALSE(migrator.CanMigrate("other", 1, 2));
}

static void test_Migrator_LatestVersion() {
    SchemaMigrator migrator;
    migrator.Register("fmt", 1, 2, [](MigrationContext&) {});
    migrator.Register("fmt", 2, 3, [](MigrationContext&) {});
    TEST_ASSERT_EQUAL_UINT32(3, migrator.LatestVersion("fmt"));
    TEST_ASSERT_EQUAL_UINT32(0, migrator.LatestVersion("unknown"));
}

// -- MigrationContext tests --

static void test_MigrationContext_Operations() {
    MigrationContext::KVMap data = {
        {"keep", "1"}, {"rename_me", "2"}, {"remove_me", "3"}
    };
    MigrationContext ctx(data);

    TEST_ASSERT_TRUE(ctx.Has("keep"));
    TEST_ASSERT_EQUAL_STRING("1", ctx.Get("keep").c_str());

    TEST_ASSERT_TRUE(ctx.Rename("rename_me", "renamed"));
    TEST_ASSERT_FALSE(ctx.Has("rename_me"));
    TEST_ASSERT_EQUAL_STRING("2", ctx.Get("renamed").c_str());

    TEST_ASSERT_TRUE(ctx.Remove("remove_me"));
    TEST_ASSERT_FALSE(ctx.Has("remove_me"));
    TEST_ASSERT_FALSE(ctx.Remove("nonexistent"));

    ctx.SetDefault("keep", "should_not_overwrite");
    TEST_ASSERT_EQUAL_STRING("1", ctx.Get("keep").c_str());

    ctx.SetDefault("new_key", "new_val");
    TEST_ASSERT_EQUAL_STRING("new_val", ctx.Get("new_key").c_str());

    ctx.Set("keep", "overwritten");
    TEST_ASSERT_EQUAL_STRING("overwritten", ctx.Get("keep").c_str());
}

void TestSchemaVersion::RunAllTests() {
    RUN_TEST(test_WriteAndReadSchemaHeader);
    RUN_TEST(test_ReadSchemaHeader_WrongMagic);
    RUN_TEST(test_ParseTextSchemaVersion_Valid);
    RUN_TEST(test_ParseTextSchemaVersion_WrongFormat);
    RUN_TEST(test_ParseTextSchemaVersion_NoHeader);
    RUN_TEST(test_MakeTextSchemaHeader);
    RUN_TEST(test_Migrator_ChainSteps);
    RUN_TEST(test_Migrator_SameVersion);
    RUN_TEST(test_Migrator_DowngradeRejected);
    RUN_TEST(test_Migrator_MissingStep);
    RUN_TEST(test_Migrator_CanMigrate);
    RUN_TEST(test_Migrator_LatestVersion);
    RUN_TEST(test_MigrationContext_Operations);
}
