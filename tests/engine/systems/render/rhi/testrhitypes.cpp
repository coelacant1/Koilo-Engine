// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testrhitypes.cpp
 * @brief RHI type system smoke tests - verifies handles, formats, flags,
 *        and descriptor struct defaults compile and behave correctly.
 */
#include "testrhitypes.hpp"
#include <koilo/systems/render/rhi/rhi_types.hpp>
#include <koilo/systems/render/rhi/rhi_caps.hpp>
#include <koilo/systems/render/rhi/rhi_device.hpp>

using namespace koilo::rhi;

// -- Handle tests ----------------------------------------------------

void TestRHITypes::TestHandleNullSentinel() {
    RHIBuffer buf{};
    RHITexture tex{};
    RHIShader shd{};
    RHIPipeline pipe{};
    RHIRenderPass pass{};
    RHIDescriptorSet ds{};
    RHIFramebuffer fb{};

    TEST_ASSERT_FALSE(buf.IsValid());
    TEST_ASSERT_FALSE(tex.IsValid());
    TEST_ASSERT_FALSE(shd.IsValid());
    TEST_ASSERT_FALSE(pipe.IsValid());
    TEST_ASSERT_FALSE(pass.IsValid());
    TEST_ASSERT_FALSE(ds.IsValid());
    TEST_ASSERT_FALSE(fb.IsValid());

    TEST_ASSERT_EQUAL_UINT32(0, buf.id);
    TEST_ASSERT_EQUAL_UINT32(0, tex.id);
}

void TestRHITypes::TestHandleEquality() {
    RHIBuffer a{1}, b{1}, c{2};
    TEST_ASSERT_TRUE(a == b);
    TEST_ASSERT_FALSE(a == c);
    TEST_ASSERT_TRUE(a != c);
    TEST_ASSERT_FALSE(a != b);

    RHITexture ta{5}, tb{5}, tc{0};
    TEST_ASSERT_TRUE(ta == tb);
    TEST_ASSERT_TRUE(ta != tc);
    TEST_ASSERT_TRUE(ta.IsValid());
    TEST_ASSERT_FALSE(tc.IsValid());
}

// -- Format tests ----------------------------------------------------

void TestRHITypes::TestFormatBytesPerPixel() {
    TEST_ASSERT_EQUAL_UINT8(1,  RHIFormatBytesPerPixel(RHIFormat::R8_Unorm));
    TEST_ASSERT_EQUAL_UINT8(4,  RHIFormatBytesPerPixel(RHIFormat::RGBA8_Unorm));
    TEST_ASSERT_EQUAL_UINT8(4,  RHIFormatBytesPerPixel(RHIFormat::BGRA8_Unorm));
    TEST_ASSERT_EQUAL_UINT8(8,  RHIFormatBytesPerPixel(RHIFormat::RGBA16F));
    TEST_ASSERT_EQUAL_UINT8(16, RHIFormatBytesPerPixel(RHIFormat::RGBA32F));
    TEST_ASSERT_EQUAL_UINT8(4,  RHIFormatBytesPerPixel(RHIFormat::D32F));
    TEST_ASSERT_EQUAL_UINT8(0,  RHIFormatBytesPerPixel(RHIFormat::Undefined));
}

void TestRHITypes::TestFormatIsDepth() {
    TEST_ASSERT_TRUE(RHIFormatIsDepth(RHIFormat::D16_Unorm));
    TEST_ASSERT_TRUE(RHIFormatIsDepth(RHIFormat::D24_Unorm_S8_Uint));
    TEST_ASSERT_TRUE(RHIFormatIsDepth(RHIFormat::D32F));
    TEST_ASSERT_TRUE(RHIFormatIsDepth(RHIFormat::D32F_S8_Uint));
    TEST_ASSERT_FALSE(RHIFormatIsDepth(RHIFormat::RGBA8_Unorm));
    TEST_ASSERT_FALSE(RHIFormatIsDepth(RHIFormat::Undefined));

    TEST_ASSERT_TRUE(RHIFormatHasStencil(RHIFormat::D24_Unorm_S8_Uint));
    TEST_ASSERT_TRUE(RHIFormatHasStencil(RHIFormat::D32F_S8_Uint));
    TEST_ASSERT_FALSE(RHIFormatHasStencil(RHIFormat::D32F));
}

// -- Flag tests ------------------------------------------------------

void TestRHITypes::TestBufferUsageFlags() {
    auto combined = RHIBufferUsage::Vertex | RHIBufferUsage::Index;
    TEST_ASSERT_TRUE(HasFlag(combined, RHIBufferUsage::Vertex));
    TEST_ASSERT_TRUE(HasFlag(combined, RHIBufferUsage::Index));
    TEST_ASSERT_FALSE(HasFlag(combined, RHIBufferUsage::Uniform));
    TEST_ASSERT_FALSE(HasFlag(combined, RHIBufferUsage::Storage));

    auto single = RHIBufferUsage::Staging;
    TEST_ASSERT_TRUE(HasFlag(single, RHIBufferUsage::Staging));
    TEST_ASSERT_FALSE(HasFlag(single, RHIBufferUsage::Vertex));
}

void TestRHITypes::TestTextureUsageFlags() {
    auto combined = RHITextureUsage::Sampled | RHITextureUsage::TransferDst;
    TEST_ASSERT_TRUE(HasFlag(combined, RHITextureUsage::Sampled));
    TEST_ASSERT_TRUE(HasFlag(combined, RHITextureUsage::TransferDst));
    TEST_ASSERT_FALSE(HasFlag(combined, RHITextureUsage::RenderTarget));
    TEST_ASSERT_FALSE(HasFlag(combined, RHITextureUsage::DepthStencil));
}

// -- Descriptor struct defaults --------------------------------------

void TestRHITypes::TestBufferDescDefaults() {
    RHIBufferDesc desc{};
    TEST_ASSERT_EQUAL_UINT64(0, desc.size);
    TEST_ASSERT_FALSE(desc.hostVisible);
    TEST_ASSERT_NULL(desc.debugName);
}

void TestRHITypes::TestTextureDescDefaults() {
    RHITextureDesc desc{};
    TEST_ASSERT_EQUAL_UINT32(1, desc.width);
    TEST_ASSERT_EQUAL_UINT32(1, desc.height);
    TEST_ASSERT_EQUAL_UINT32(1, desc.depth);
    TEST_ASSERT_EQUAL_UINT32(1, desc.mipLevels);
    TEST_ASSERT_EQUAL_UINT32(1, desc.arrayLayers);
    TEST_ASSERT_TRUE(desc.format == RHIFormat::RGBA8_Unorm);
}

void TestRHITypes::TestPipelineDescDefaults() {
    RHIPipelineDesc desc{};
    TEST_ASSERT_FALSE(desc.vertexShader.IsValid());
    TEST_ASSERT_FALSE(desc.fragmentShader.IsValid());
    TEST_ASSERT_FALSE(desc.renderPass.IsValid());
    TEST_ASSERT_EQUAL_UINT32(0, desc.vertexAttrCount);
    TEST_ASSERT_EQUAL_UINT32(0, desc.vertexStride);
    TEST_ASSERT_TRUE(desc.topology == RHITopology::TriangleList);
    TEST_ASSERT_TRUE(desc.rasterizer.cullMode == RHICullMode::Back);
    TEST_ASSERT_TRUE(desc.depthStencil.depthTest);
    TEST_ASSERT_TRUE(desc.depthStencil.depthWrite);
    TEST_ASSERT_FALSE(desc.blend.enabled);
}

void TestRHITypes::TestRenderPassDescDefaults() {
    RHIRenderPassDesc desc{};
    TEST_ASSERT_EQUAL_UINT32(0, desc.colorAttachmentCount);
    TEST_ASSERT_TRUE(desc.hasDepth);
    TEST_ASSERT_TRUE(desc.depthFormat == RHIFormat::D24_Unorm_S8_Uint);
}

// -- Capability tests ------------------------------------------------

void TestRHITypes::TestCapabilityFeatureFlags() {
    auto combined = RHIFeature::PushConstants | RHIFeature::StorageBuffers;
    TEST_ASSERT_TRUE(HasFeature(combined, RHIFeature::PushConstants));
    TEST_ASSERT_TRUE(HasFeature(combined, RHIFeature::StorageBuffers));
    TEST_ASSERT_FALSE(HasFeature(combined, RHIFeature::ComputeShaders));
    TEST_ASSERT_FALSE(HasFeature(combined, RHIFeature::TimestampQueries));
}

void TestRHITypes::TestLimitsDefaults() {
    RHILimits limits{};
    TEST_ASSERT_EQUAL_UINT32(4096, limits.maxTextureSize);
    TEST_ASSERT_EQUAL_UINT32(4, limits.maxColorAttachments);
    TEST_ASSERT_EQUAL_UINT32(16, limits.maxVertexAttributes);
    TEST_ASSERT_EQUAL_UINT32(65536, limits.maxUniformBufferSize);
    TEST_ASSERT_EQUAL_UINT32(128, limits.maxPushConstantSize);
    TEST_ASSERT_EQUAL_UINT64(0, limits.totalDeviceMemory);
}

// -- Test runner -----------------------------------------------------

void TestRHITypes::RunAllTests() {
    RUN_TEST(TestHandleNullSentinel);
    RUN_TEST(TestHandleEquality);
    RUN_TEST(TestFormatBytesPerPixel);
    RUN_TEST(TestFormatIsDepth);
    RUN_TEST(TestBufferUsageFlags);
    RUN_TEST(TestTextureUsageFlags);
    RUN_TEST(TestBufferDescDefaults);
    RUN_TEST(TestTextureDescDefaults);
    RUN_TEST(TestPipelineDescDefaults);
    RUN_TEST(TestRenderPassDescDefaults);
    RUN_TEST(TestCapabilityFeatureFlags);
    RUN_TEST(TestLimitsDefaults);
}
