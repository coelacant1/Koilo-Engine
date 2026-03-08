// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testscenedata.cpp
 * @brief Implementation of SceneData structure tests.
 */

#include "testscenedata.hpp"
#include <cstring>

using namespace koilo;

// ========== Structure Size Tests ==========

void TestSceneData::TestStructureSizes() {
    // Verify expected sizes for portability
    
    // Basic types
    TEST_ASSERT_EQUAL(12, sizeof(Vector3DData));      // 3 floats
    TEST_ASSERT_EQUAL(16, sizeof(QuaternionData));    // 4 floats
    TEST_ASSERT_EQUAL(40, sizeof(TransformData));     // Vec3 + Quat + Vec3
    TEST_ASSERT_EQUAL(6, sizeof(IndexGroupData));     // 3 uint16_t
    TEST_ASSERT_EQUAL(3, sizeof(Color888Data));       // 3 uint8_t
    
    // Geometry
    TEST_ASSERT_EQUAL(16, sizeof(TriangleGroupData)); // 2 uint32_t + 2 offsets
    
    // Materials
    TEST_ASSERT_TRUE(sizeof(MaterialData) >= 72);     // Type + union
    
    // Mesh/Camera/Display
    TEST_ASSERT_EQUAL(56, sizeof(MeshData));          // 3 IDs + transform
    TEST_ASSERT_EQUAL(64, sizeof(CameraData));        // Transform + params
    TEST_ASSERT_TRUE(sizeof(DisplayBackendData) >= 144); // Type + union
    
    // Animation
    TEST_ASSERT_EQUAL(12, sizeof(KeyframeData));      // time + value + interp
    TEST_ASSERT_TRUE(sizeof(AnimationTrackData) >= 20); // Target + keyframes
}

void TestSceneData::TestHeaderSize() {
    // Header should be reasonable size
    TEST_ASSERT_TRUE(sizeof(CompiledSceneHeader) >= 192);
    TEST_ASSERT_TRUE(sizeof(CompiledSceneHeader) < 512);
}

void TestSceneData::TestAlignment() {
    // Verify proper alignment for performance
    
    // Float data should be 4-byte aligned
    TEST_ASSERT_EQUAL(0, sizeof(Vector3DData) % 4);
    TEST_ASSERT_EQUAL(0, sizeof(QuaternionData) % 4);
    TEST_ASSERT_EQUAL(0, sizeof(TransformData) % 4);
    
    // Structures should be aligned
    TEST_ASSERT_EQUAL(0, sizeof(MaterialData) % 4);
    TEST_ASSERT_EQUAL(0, sizeof(MeshData) % 4);
    TEST_ASSERT_EQUAL(0, sizeof(CameraData) % 4);
}

// ========== Data Initialization Tests ==========

void TestSceneData::TestVector3DData() {
    Vector3DData vec;
    vec.x = 1.0f;
    vec.y = 2.0f;
    vec.z = 3.0f;
    
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, vec.x);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f, vec.y);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.0f, vec.z);
}

void TestSceneData::TestQuaternionData() {
    QuaternionData quat;
    quat.x = 0.0f;
    quat.y = 0.0f;
    quat.z = 0.0f;
    quat.w = 1.0f; // Identity quaternion
    
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, quat.x);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, quat.y);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, quat.z);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, quat.w);
}

void TestSceneData::TestTransformData() {
    TransformData transform;
    
    // Position
    transform.position.x = 10.0f;
    transform.position.y = 20.0f;
    transform.position.z = 30.0f;
    
    // Rotation (identity)
    transform.rotation.x = 0.0f;
    transform.rotation.y = 0.0f;
    transform.rotation.z = 0.0f;
    transform.rotation.w = 1.0f;
    
    // Scale
    transform.scale.x = 1.0f;
    transform.scale.y = 1.0f;
    transform.scale.z = 1.0f;
    
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, transform.position.x);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 20.0f, transform.position.y);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 30.0f, transform.position.z);
}

void TestSceneData::TestIndexGroupData() {
    IndexGroupData indices;
    indices.a = 0;
    indices.b = 1;
    indices.c = 2;
    
    TEST_ASSERT_EQUAL(0, indices.a);
    TEST_ASSERT_EQUAL(1, indices.b);
    TEST_ASSERT_EQUAL(2, indices.c);
}

void TestSceneData::TestTriangleGroupData() {
    TriangleGroupData geom;
    geom.vertexCount = 100;
    geom.indexCount = 50;
    geom.verticesOffset = 1000;
    geom.indicesOffset = 2000;
    
    TEST_ASSERT_EQUAL(100, geom.vertexCount);
    TEST_ASSERT_EQUAL(50, geom.indexCount);
    TEST_ASSERT_EQUAL(1000, geom.verticesOffset);
    TEST_ASSERT_EQUAL(2000, geom.indicesOffset);
}

// ========== Material Data Tests ==========

void TestSceneData::TestMaterialData_UniformColor() {
    MaterialData mat;
    mat.type = MaterialType::UniformColor;
    mat.color.r = 255;
    mat.color.g = 128;
    mat.color.b = 64;
    
    TEST_ASSERT_EQUAL(MaterialType::UniformColor, mat.type);
    TEST_ASSERT_EQUAL(255, mat.color.r);
    TEST_ASSERT_EQUAL(128, mat.color.g);
    TEST_ASSERT_EQUAL(64, mat.color.b);
}

void TestSceneData::TestMaterialData_PhongLight() {
    MaterialData mat;
    mat.type = MaterialType::PhongLight;
    mat.ambient.r = 50;
    mat.ambient.g = 50;
    mat.ambient.b = 50;
    mat.diffuse.r = 200;
    mat.diffuse.g = 200;
    mat.diffuse.b = 200;
    mat.specular.r = 255;
    mat.specular.g = 255;
    mat.specular.b = 255;
    mat.shininess = 32.0f;
    
    TEST_ASSERT_EQUAL(MaterialType::PhongLight, mat.type);
    TEST_ASSERT_EQUAL(50, mat.ambient.r);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 32.0f, mat.shininess);
}

void TestSceneData::TestMaterialData_Union() {
    MaterialData mat;
    mat.type = MaterialType::UniformColor;
    mat.color.r = 100;
    TEST_ASSERT_EQUAL(100, mat.color.r);
    
    mat.type = MaterialType::PhongLight;
    mat.shininess = 16.0f;
    TEST_ASSERT_EQUAL(MaterialType::PhongLight, mat.type);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 16.0f, mat.shininess);
}

// ========== Display Backend Tests ==========

void TestSceneData::TestDisplayBackendData_HUB75() {
    DisplayBackendData display;
    display.type = DisplayBackendType::HUB75;
    display.width = 64;
    display.height = 32;
    display.cameraIndex = 0;
    display.r1_pin = 2;
    display.g1_pin = 3;
    display.b1_pin = 4;
    display.clk_pin = 13;
    display.hub75_brightness = 128;
    
    TEST_ASSERT_EQUAL(DisplayBackendType::HUB75, display.type);
    TEST_ASSERT_EQUAL(64, display.width);
    TEST_ASSERT_EQUAL(32, display.height);
    TEST_ASSERT_EQUAL(2, display.r1_pin);
    TEST_ASSERT_EQUAL(13, display.clk_pin);
}

void TestSceneData::TestDisplayBackendData_SPI() {
    DisplayBackendData display;
    display.type = DisplayBackendType::SPI_ILI9341;
    display.width = 128;
    display.height = 64;
    display.cameraIndex = 0;
    display.spi_cs_pin = 10;
    display.spi_dc_pin = 9;
    display.spi_rst_pin = 8;
    display.spi_frequency = 40000000;
    
    TEST_ASSERT_EQUAL(DisplayBackendType::SPI_ILI9341, display.type);
    TEST_ASSERT_EQUAL(128, display.width);
    TEST_ASSERT_EQUAL(10, display.spi_cs_pin);
    TEST_ASSERT_EQUAL(40000000, display.spi_frequency);
}

void TestSceneData::TestDisplayBackendData_Union() {
    DisplayBackendData display;
    display.type = DisplayBackendType::HUB75;
    display.r1_pin = 2;
    TEST_ASSERT_EQUAL(2, display.r1_pin);
    
    display.type = DisplayBackendType::SPI_ILI9341;
    display.spi_cs_pin = 10;
    TEST_ASSERT_EQUAL(DisplayBackendType::SPI_ILI9341, display.type);
    TEST_ASSERT_EQUAL(10, display.spi_cs_pin);
}

// ========== Scene Header Tests ==========

void TestSceneData::TestCompiledSceneHeader_Magic() {
    CompiledSceneHeader header;
    header.magic = KOILOSCENE_MAGIC;
    header.version = KOILOSCENE_VERSION;
    
    TEST_ASSERT_EQUAL(KOILOSCENE_MAGIC, header.magic);
    TEST_ASSERT_EQUAL(KOILOSCENE_VERSION, header.version);
    
    // Verify magic is the expected value
    TEST_ASSERT_EQUAL(0x4B4F494C, KOILOSCENE_MAGIC); // "KOIS"
    TEST_ASSERT_EQUAL(0x00000001, KOILOSCENE_VERSION); // Version 1
}

void TestSceneData::TestCompiledSceneHeader_Offsets() {
    CompiledSceneHeader header;
    std::memset(&header, 0, sizeof(header));
    
    header.geometriesOffset = 1000;
    header.materialsOffset = 2000;
    header.meshesOffset = 3000;
    header.camerasOffset = 4000;
    header.displaysOffset = 5000;
    
    TEST_ASSERT_EQUAL(1000, header.geometriesOffset);
    TEST_ASSERT_EQUAL(2000, header.materialsOffset);
    TEST_ASSERT_EQUAL(3000, header.meshesOffset);
    TEST_ASSERT_EQUAL(4000, header.camerasOffset);
    TEST_ASSERT_EQUAL(5000, header.displaysOffset);
}

void TestSceneData::TestCompiledSceneHeader_Sizes() {
    CompiledSceneHeader header;
    std::memset(&header, 0, sizeof(header));
    
    // Set counts
    header.geometryCount = 10;
    header.materialCount = 5;
    header.meshCount = 8;
    header.cameraCount = 2;
    header.displayCount = 1;
    
    TEST_ASSERT_EQUAL(10, header.geometryCount);
    TEST_ASSERT_EQUAL(5, header.materialCount);
    TEST_ASSERT_EQUAL(8, header.meshCount);
    TEST_ASSERT_EQUAL(2, header.cameraCount);
    TEST_ASSERT_EQUAL(1, header.displayCount);
}

// ========== Constants Tests ==========

void TestSceneData::TestConstants() {
    // Verify constants are defined
    TEST_ASSERT_EQUAL(0x4B4F494C, KOILOSCENE_MAGIC);
    TEST_ASSERT_EQUAL(0x00000001, KOILOSCENE_VERSION);
    
    // Verify material type enum values
    TEST_ASSERT_TRUE(static_cast<uint8_t>(MaterialType::UniformColor) == 0);
    TEST_ASSERT_TRUE(static_cast<uint8_t>(MaterialType::PhongLight) == 1);
    
    // Verify display backend enum values
    TEST_ASSERT_TRUE(static_cast<uint8_t>(DisplayBackendType::Null) == 102);
    TEST_ASSERT_TRUE(static_cast<uint8_t>(DisplayBackendType::HUB75) == 0);
}

// ========== Test Runner ==========

void TestSceneData::RunAllTests() {
    RUN_TEST(TestStructureSizes);
    RUN_TEST(TestHeaderSize);
    RUN_TEST(TestAlignment);
    
    RUN_TEST(TestVector3DData);
    RUN_TEST(TestQuaternionData);
    RUN_TEST(TestTransformData);
    RUN_TEST(TestIndexGroupData);
    RUN_TEST(TestTriangleGroupData);
    
    RUN_TEST(TestMaterialData_UniformColor);
    RUN_TEST(TestMaterialData_PhongLight);
    RUN_TEST(TestMaterialData_Union);
    
    RUN_TEST(TestDisplayBackendData_HUB75);
    RUN_TEST(TestDisplayBackendData_SPI);
    RUN_TEST(TestDisplayBackendData_Union);
    
    RUN_TEST(TestCompiledSceneHeader_Magic);
    RUN_TEST(TestCompiledSceneHeader_Offsets);
    RUN_TEST(TestCompiledSceneHeader_Sizes);
    
    RUN_TEST(TestConstants);
}
