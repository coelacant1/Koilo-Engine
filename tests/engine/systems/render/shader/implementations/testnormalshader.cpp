/**
 * @file testnormalshader.cpp
 * @brief Implementation of NormalShader unit tests.
 */

#include "testnormalshader.hpp"
#include <utils/testhelpers.hpp>

// Mock material for testing (NormalShader doesn't use material params)
class MockMaterial : public IMaterial {
public:
    MockMaterial() : IMaterial(nullptr) {}
};

// ========== Constructor Tests ==========

void TestNormalShader::TestDefaultConstructor() {
    // NormalShader has no explicit constructors, uses compiler-generated default
    NormalShader shader;

    // Should be able to create instance without parameters
    TEST_ASSERT_TRUE(true);
}

// ========== Method Tests ==========

void TestNormalShader::TestShade() {
    NormalShader shader;
    MockMaterial material;

    // Test with a standard upward-facing normal (0, 0, 1)
    Vector3D position(0.0f, 0.0f, 0.0f);
    Vector3D normal(0.0f, 0.0f, 1.0f);  // Unit normal pointing up
    Vector3D uvw(0.0f, 0.0f, 0.0f);

    SurfaceProperties surf(position, normal, uvw);
    RGBColor result = shader.Shade(surf, material);

    // Normal (0, 0, 1) -> normalized stays (0, 0, 1)
    // Remap: (0 + 1) * 0.5 * 255 = 0.5 * 255 = 127.5
    //        (0 + 1) * 0.5 * 255 = 0.5 * 255 = 127.5
    //        (1 + 1) * 0.5 * 255 = 1.0 * 255 = 255
    RGBColor expected(127, 127, 255);

    TEST_ASSERT_RGB_EQUAL(expected, result);
}

// ========== Edge Cases ==========

void TestNormalShader::TestParameterizedConstructor() {
    // NormalShader has no parameterized constructor, only default
    NormalShader shader;

    TEST_ASSERT_TRUE(true);
}

void TestNormalShader::TestEdgeCases() {
    NormalShader shader;
    MockMaterial material;

    // Test with all negative components
    Vector3D position(0.0f, 0.0f, 0.0f);
    Vector3D normal(-1.0f, -1.0f, -1.0f);
    Vector3D uvw(0.0f, 0.0f, 0.0f);

    SurfaceProperties surf(position, normal, uvw);
    RGBColor result = shader.Shade(surf, material);

    // After normalization: (-0.577, -0.577, -0.577)
    // Remapped: (-0.577 + 1) * 0.5 * 255 = 0.423 * 0.5 * 255 ≈ 54
    TEST_ASSERT_TRUE(result.R >= 53 && result.R <= 55);
    TEST_ASSERT_TRUE(result.G >= 53 && result.G <= 55);
    TEST_ASSERT_TRUE(result.B >= 53 && result.B <= 55);
}

// ========== Test Runner ==========

void TestNormalShader::RunAllTests() {
    RUN_TEST(TestDefaultConstructor);
    RUN_TEST(TestParameterizedConstructor);
    RUN_TEST(TestShade);

    RUN_TEST(TestEdgeCases);

}
