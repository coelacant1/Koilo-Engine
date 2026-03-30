// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file testsoftwarerhi.cpp
 * @brief Unit tests for SoftwareRHIDevice.
 */
#include "testsoftwarerhi.hpp"
#include <koilo/systems/render/rhi/software/software_rhi_device.hpp>
#include <cstring>

using namespace koilo::rhi;

namespace TestSoftwareRHI {

// -- Lifecycle -------------------------------------------------------

void TestInitializeAndShutdown() {
    SoftwareRHIDevice dev;
    TEST_ASSERT_TRUE(dev.Initialize());
    dev.Shutdown();
    // Double shutdown should be safe
    dev.Shutdown();
    // Re-initialize after shutdown
    TEST_ASSERT_TRUE(dev.Initialize());
    dev.Shutdown();
}

void TestGetNameAndCaps() {
    SoftwareRHIDevice dev;
    dev.Initialize();
    TEST_ASSERT_EQUAL_STRING("Software RHI", dev.GetName());
    TEST_ASSERT_FALSE(dev.SupportsFeature(RHIFeature::TimestampQueries));
    TEST_ASSERT_FALSE(dev.SupportsFeature(RHIFeature::ComputeShaders));

    RHILimits lim = dev.GetLimits();
    TEST_ASSERT_GREATER_THAN(0u, lim.maxTextureSize);
    TEST_ASSERT_GREATER_THAN(0u, lim.maxVertexAttributes);
    dev.Shutdown();
}

// -- Buffers ---------------------------------------------------------

void TestCreateDestroyBuffer() {
    SoftwareRHIDevice dev;
    dev.Initialize();

    RHIBufferDesc desc;
    desc.size = 256;
    desc.usage = RHIBufferUsage::Vertex;

    RHIBuffer buf = dev.CreateBuffer(desc);
    TEST_ASSERT_TRUE(buf.IsValid());

    // Destroy and verify handle pattern (no crash)
    dev.DestroyBuffer(buf);
    // Double destroy should be safe
    dev.DestroyBuffer(buf);
    dev.Shutdown();
}

void TestUpdateAndMapBuffer() {
    SoftwareRHIDevice dev;
    dev.Initialize();

    RHIBufferDesc desc;
    desc.size = 64;
    desc.usage = RHIBufferUsage::Uniform;
    desc.hostVisible = true;

    RHIBuffer buf = dev.CreateBuffer(desc);

    // Write data via UpdateBuffer
    float data[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    dev.UpdateBuffer(buf, data, sizeof(data), 0);

    // Read back via MapBuffer
    void* mapped = dev.MapBuffer(buf);
    TEST_ASSERT_NOT_NULL(mapped);

    float* readBack = static_cast<float*>(mapped);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, readBack[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 4.0f, readBack[3]);

    dev.UnmapBuffer(buf);

    // Update at offset
    float more = 99.0f;
    dev.UpdateBuffer(buf, &more, sizeof(float), 2 * sizeof(float));
    mapped = dev.MapBuffer(buf);
    readBack = static_cast<float*>(mapped);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 99.0f, readBack[2]);
    dev.UnmapBuffer(buf);

    dev.DestroyBuffer(buf);
    dev.Shutdown();
}

// -- Textures --------------------------------------------------------

void TestCreateDestroyTexture() {
    SoftwareRHIDevice dev;
    dev.Initialize();

    RHITextureDesc desc;
    desc.width  = 64;
    desc.height = 64;
    desc.format = RHIFormat::RGBA8_Unorm;

    RHITexture tex = dev.CreateTexture(desc);
    TEST_ASSERT_TRUE(tex.IsValid());

    dev.DestroyTexture(tex);
    dev.DestroyTexture(tex); // double destroy safe
    dev.Shutdown();
}

void TestUpdateTexture() {
    SoftwareRHIDevice dev;
    dev.Initialize();

    RHITextureDesc desc;
    desc.width  = 2;
    desc.height = 2;
    desc.format = RHIFormat::RGBA8_Unorm;

    RHITexture tex = dev.CreateTexture(desc);

    // 2x2 RGBA8 = 16 bytes
    uint8_t pixels[16];
    std::memset(pixels, 0xAB, sizeof(pixels));
    dev.UpdateTexture(tex, pixels, sizeof(pixels), 2, 2);

    dev.DestroyTexture(tex);
    dev.Shutdown();
}

// -- Shaders ---------------------------------------------------------

void TestCreateDestroyShader() {
    SoftwareRHIDevice dev;
    dev.Initialize();

    const char* dummyCode = "dummy shader bytecode";
    RHIShader vs = dev.CreateShader(RHIShaderStage::Vertex,
                                     dummyCode, strlen(dummyCode));
    TEST_ASSERT_TRUE(vs.IsValid());

    RHIShader fs = dev.CreateShader(RHIShaderStage::Fragment,
                                     dummyCode, strlen(dummyCode));
    TEST_ASSERT_TRUE(fs.IsValid());
    TEST_ASSERT_NOT_EQUAL(vs.id, fs.id);

    dev.DestroyShader(vs);
    dev.DestroyShader(fs);
    dev.Shutdown();
}

// -- Pipelines -------------------------------------------------------

void TestCreateDestroyPipeline() {
    SoftwareRHIDevice dev;
    dev.Initialize();

    RHIPipelineDesc desc;
    desc.topology = RHITopology::TriangleList;
    desc.vertexStride = 32;

    RHIPipeline pip = dev.CreatePipeline(desc);
    TEST_ASSERT_TRUE(pip.IsValid());

    dev.DestroyPipeline(pip);
    dev.Shutdown();
}

// -- Render passes ---------------------------------------------------

void TestCreateDestroyRenderPass() {
    SoftwareRHIDevice dev;
    dev.Initialize();

    RHIRenderPassDesc desc;
    desc.colorAttachmentCount = 1;
    desc.colorAttachments[0].format = RHIFormat::RGBA8_Unorm;
    desc.hasDepth = true;
    desc.depthFormat = RHIFormat::D32F;

    RHIRenderPass rp = dev.CreateRenderPass(desc);
    TEST_ASSERT_TRUE(rp.IsValid());

    dev.DestroyRenderPass(rp);
    dev.Shutdown();
}

// -- Framebuffers ----------------------------------------------------

void TestCreateDestroyFramebuffer() {
    SoftwareRHIDevice dev;
    dev.Initialize();

    RHIRenderPassDesc rpDesc;
    rpDesc.colorAttachmentCount = 1;
    rpDesc.hasDepth = true;
    rpDesc.depthFormat = RHIFormat::D32F;
    RHIRenderPass rp = dev.CreateRenderPass(rpDesc);

    RHITextureDesc colorDesc;
    colorDesc.width = 64; colorDesc.height = 64;
    colorDesc.format = RHIFormat::RGBA8_Unorm;
    RHITexture color = dev.CreateTexture(colorDesc);

    RHITextureDesc depthDesc;
    depthDesc.width = 64; depthDesc.height = 64;
    depthDesc.format = RHIFormat::D32F;
    RHITexture depth = dev.CreateTexture(depthDesc);

    RHIFramebuffer fbo = dev.CreateFramebuffer(rp, &color, 1, depth, 64, 64);
    TEST_ASSERT_TRUE(fbo.IsValid());

    dev.DestroyFramebuffer(fbo);
    dev.DestroyTexture(color);
    dev.DestroyTexture(depth);
    dev.DestroyRenderPass(rp);
    dev.Shutdown();
}

// -- Swapchain -------------------------------------------------------

void TestSwapchainRenderPass() {
    SoftwareRHIDevice dev;
    dev.Initialize();

    RHIRenderPass rp = dev.GetSwapchainRenderPass();
    TEST_ASSERT_TRUE(rp.IsValid());

    dev.Shutdown();
}

void TestOnResizeAllocatesSwapchain() {
    SoftwareRHIDevice dev;
    dev.Initialize();

    dev.OnResize(320, 240);

    uint32_t w = 0, h = 0;
    dev.GetSwapchainSize(w, h);
    TEST_ASSERT_EQUAL_UINT32(320, w);
    TEST_ASSERT_EQUAL_UINT32(240, h);

    const uint8_t* pixels = dev.GetSwapchainPixels();
    TEST_ASSERT_NOT_NULL(pixels);

    // Resize again
    dev.OnResize(640, 480);
    dev.GetSwapchainSize(w, h);
    TEST_ASSERT_EQUAL_UINT32(640, w);
    TEST_ASSERT_EQUAL_UINT32(480, h);

    dev.Shutdown();
}

// -- Render pass clearing --------------------------------------------

void TestBeginRenderPassClearsColor() {
    SoftwareRHIDevice dev;
    dev.Initialize();

    // Create a small framebuffer with color attachment
    RHIRenderPassDesc rpDesc;
    rpDesc.colorAttachmentCount = 1;
    rpDesc.colorAttachments[0].format = RHIFormat::RGBA8_Unorm;
    rpDesc.hasDepth = false;
    RHIRenderPass rp = dev.CreateRenderPass(rpDesc);

    RHITextureDesc texDesc;
    texDesc.width = 4; texDesc.height = 4;
    texDesc.format = RHIFormat::RGBA8_Unorm;
    RHITexture color = dev.CreateTexture(texDesc);

    RHITexture noDepth = {};
    RHIFramebuffer fbo = dev.CreateFramebuffer(rp, &color, 1, noDepth, 4, 4);

    // Begin render pass with red clear color
    RHIClearValue clear;
    clear.color[0] = 1.0f; clear.color[1] = 0.0f;
    clear.color[2] = 0.0f; clear.color[3] = 1.0f;

    dev.BeginFrame();
    dev.BeginRenderPass(rp, fbo, clear);

    // Map the color texture and verify it was cleared to red
    // (We access via MapBuffer-equivalent... need to use UpdateTexture API)
    // Since we can't directly map textures, we verify the device didn't crash.
    // Full pixel verification will be tested in SW-2 when Draw() writes pixels.

    dev.EndRenderPass();
    dev.EndFrame();

    dev.DestroyFramebuffer(fbo);
    dev.DestroyTexture(color);
    dev.DestroyRenderPass(rp);
    dev.Shutdown();
}

void TestBeginSwapchainRenderPassClears() {
    SoftwareRHIDevice dev;
    dev.Initialize();
    dev.OnResize(4, 4);

    RHIClearValue clear;
    clear.color[0] = 0.0f; clear.color[1] = 1.0f;
    clear.color[2] = 0.0f; clear.color[3] = 1.0f;

    dev.BeginSwapchainRenderPass(clear);

    const uint8_t* pixels = dev.GetSwapchainPixels();
    TEST_ASSERT_NOT_NULL(pixels);
    // First pixel should be green (0, 255, 0, 255)
    TEST_ASSERT_EQUAL_UINT8(0,   pixels[0]);
    TEST_ASSERT_EQUAL_UINT8(255, pixels[1]);
    TEST_ASSERT_EQUAL_UINT8(0,   pixels[2]);
    TEST_ASSERT_EQUAL_UINT8(255, pixels[3]);

    dev.Shutdown();
}

// -- Bind state tracking ---------------------------------------------

void TestBindStateTracking() {
    SoftwareRHIDevice dev;
    dev.Initialize();

    // Create resources to bind
    RHIBufferDesc bufDesc;
    bufDesc.size = 128;
    RHIBuffer vbo = dev.CreateBuffer(bufDesc);
    RHIBuffer ubo = dev.CreateBuffer(bufDesc);

    RHIPipelineDesc pipDesc;
    RHIPipeline pip = dev.CreatePipeline(pipDesc);

    RHITextureDesc texDesc;
    texDesc.width = 2; texDesc.height = 2;
    RHITexture tex = dev.CreateTexture(texDesc);

    // All bind calls should succeed without crash
    dev.BeginFrame();
    dev.BindPipeline(pip);
    dev.BindVertexBuffer(vbo, 0, 0);
    dev.BindUniformBuffer(ubo, 0, 0, 0, 128);
    dev.BindTexture(tex, 2, 0);

    RHIViewport vp = {0, 0, 640, 480, 0.0f, 1.0f};
    dev.SetViewport(vp);
    RHIScissor sc = {0, 0, 640, 480};
    dev.SetScissor(sc);
    dev.EndFrame();

    dev.DestroyBuffer(vbo);
    dev.DestroyBuffer(ubo);
    dev.DestroyPipeline(pip);
    dev.DestroyTexture(tex);
    dev.Shutdown();
}

// -- Draw (no-op in SW-1) --------------------------------------------

void TestDrawIsNoOp() {
    SoftwareRHIDevice dev;
    dev.Initialize();

    dev.BeginFrame();
    // Should not crash, just no-op
    dev.Draw(6);
    dev.Draw(100, 1, 0, 0);
    dev.DrawIndexed(36, 1, 0, 0, 0);
    dev.EndFrame();
    dev.Present();

    dev.Shutdown();
}

// -- SW-2: Rasterization tests ----------------------------------------

// Helper: build an identity 4x4 matrix at the given memory offset
static void WriteIdentityMatrix(float* dst) {
    for (int i = 0; i < 16; ++i) dst[i] = 0.0f;
    dst[0] = dst[5] = dst[10] = dst[15] = 1.0f;
}

void TestTriangleRasterWritesPixels() {
    SoftwareRHIDevice dev;
    dev.Initialize();
    dev.OnResize(8, 8);

    // Create pipeline: triangle list, no depth, no culling, 32-byte stride (RHIVertex)
    RHIPipelineDesc pipDesc;
    pipDesc.topology = RHITopology::TriangleList;
    pipDesc.vertexStride = 32;
    pipDesc.vertexAttrCount = 3; // pos, normal, uv
    pipDesc.depthStencil.depthTest = false;
    pipDesc.depthStencil.depthWrite = false;
    pipDesc.rasterizer.cullMode = RHICullMode::None;
    RHIPipeline pip = dev.CreatePipeline(pipDesc);

    // Create vertex buffer with a triangle covering center of the viewport
    // NDC triangle: (-0.5,-0.5,0), (0.5,-0.5,0), (0,0.5,0)
    // With identity MVP, these NDC coords map to screen pixels
    struct RHIVertex { float px,py,pz, nx,ny,nz, u,v; };
    RHIVertex tri[3] = {
        {-0.8f, -0.8f, 0.0f,  0,1,0,  0,0},
        { 0.8f, -0.8f, 0.0f,  0,1,0,  1,0},
        { 0.0f,  0.8f, 0.0f,  0,1,0,  0.5f,1},
    };

    RHIBufferDesc bufDesc;
    bufDesc.size = sizeof(tri);
    RHIBuffer vbo = dev.CreateBuffer(bufDesc);
    dev.UpdateBuffer(vbo, tri, sizeof(tri), 0);

    // Create UBO with identity model/view/proj matrices (336 bytes)
    float uboData[84] = {};
    WriteIdentityMatrix(uboData);      // model
    WriteIdentityMatrix(uboData + 16); // view
    WriteIdentityMatrix(uboData + 32); // proj
    RHIBufferDesc uboDesc;
    uboDesc.size = sizeof(uboData);
    RHIBuffer ubo = dev.CreateBuffer(uboDesc);
    dev.UpdateBuffer(ubo, uboData, sizeof(uboData), 0);

    // Clear swapchain to black
    RHIClearValue clear;
    clear.color[0] = 0; clear.color[1] = 0;
    clear.color[2] = 0; clear.color[3] = 1.0f;

    dev.BeginFrame();
    dev.BeginSwapchainRenderPass(clear);
    dev.BindPipeline(pip);
    dev.BindVertexBuffer(vbo, 0, 0);
    dev.BindUniformBuffer(ubo, 0, 0, 0, sizeof(uboData));

    RHIViewport vp = {0, 0, 8, 8, 0, 1};
    dev.SetViewport(vp);

    dev.Draw(3);
    dev.EndRenderPass();
    dev.EndFrame();

    // Verify: center pixel (4,4) should have been written (non-black)
    const uint8_t* pixels = dev.GetSwapchainPixels();
    TEST_ASSERT_NOT_NULL(pixels);
    size_t centerIdx = (4 * 8 + 4) * 4;
    // At least one channel should be non-zero (the triangle writes white with hemisphere lighting)
    bool pixelWritten = pixels[centerIdx] > 0 || pixels[centerIdx+1] > 0 || pixels[centerIdx+2] > 0;
    TEST_ASSERT_TRUE(pixelWritten);

    // Corner pixel (0,0) should still be black (outside the triangle)
    // Note: with the triangle vertices at +-0.8, the top-left corner should be black
    // Actually, pixel (0,0) maps to screen top-left. Let's check it.
    // The triangle is centered so corners should be background.
    size_t cornerIdx = 0;
    bool cornerBlack = pixels[cornerIdx] == 0 && pixels[cornerIdx+1] == 0 && pixels[cornerIdx+2] == 0;
    TEST_ASSERT_TRUE(cornerBlack);

    dev.DestroyBuffer(vbo);
    dev.DestroyBuffer(ubo);
    dev.DestroyPipeline(pip);
    dev.Shutdown();
}

void TestTriangleDepthTest() {
    SoftwareRHIDevice dev;
    dev.Initialize();
    dev.OnResize(8, 8);

    // Create pipeline with depth test
    RHIPipelineDesc pipDesc;
    pipDesc.topology = RHITopology::TriangleList;
    pipDesc.vertexStride = 32;
    pipDesc.vertexAttrCount = 3;
    pipDesc.depthStencil.depthTest = true;
    pipDesc.depthStencil.depthWrite = true;
    pipDesc.rasterizer.cullMode = RHICullMode::None;
    RHIPipeline pip = dev.CreatePipeline(pipDesc);

    // UBO with identity matrices
    float uboData[84] = {};
    WriteIdentityMatrix(uboData);
    WriteIdentityMatrix(uboData + 16);
    WriteIdentityMatrix(uboData + 32);
    RHIBufferDesc uboDesc;
    uboDesc.size = sizeof(uboData);
    RHIBuffer ubo = dev.CreateBuffer(uboDesc);
    dev.UpdateBuffer(ubo, uboData, sizeof(uboData), 0);

    // Triangle 1 (red-ish): at z=0.3, large covering center
    struct RHIVertex { float px,py,pz, nx,ny,nz, u,v; };
    RHIVertex tri1[3] = {
        {-0.9f, -0.9f, 0.3f,  0,1,0,  0,0},
        { 0.9f, -0.9f, 0.3f,  0,1,0,  1,0},
        { 0.0f,  0.9f, 0.3f,  0,1,0,  0.5f,1},
    };

    // Triangle 2: at z=0.8 (farther), also covers center - should be occluded
    RHIVertex tri2[3] = {
        {-0.9f, -0.9f, 0.8f,  0,1,0,  0,0},
        { 0.9f, -0.9f, 0.8f,  0,1,0,  1,0},
        { 0.0f,  0.9f, 0.8f,  0,1,0,  0.5f,1},
    };

    RHIBufferDesc bufDesc;
    bufDesc.size = sizeof(tri1);
    RHIBuffer vbo1 = dev.CreateBuffer(bufDesc);
    dev.UpdateBuffer(vbo1, tri1, sizeof(tri1), 0);
    RHIBuffer vbo2 = dev.CreateBuffer(bufDesc);
    dev.UpdateBuffer(vbo2, tri2, sizeof(tri2), 0);

    // Clear to black, depth to 1.0
    RHIClearValue clear;
    clear.color[0] = 0; clear.color[1] = 0;
    clear.color[2] = 0; clear.color[3] = 1.0f;
    clear.depth = 1.0f;

    dev.BeginFrame();
    dev.BeginSwapchainRenderPass(clear);
    dev.BindPipeline(pip);
    dev.BindUniformBuffer(ubo, 0, 0, 0, sizeof(uboData));
    RHIViewport vp = {0, 0, 8, 8, 0, 1};
    dev.SetViewport(vp);

    // Draw triangle 1 (closer, z=0.3)
    dev.BindVertexBuffer(vbo1, 0, 0);
    dev.Draw(3);

    // Read center pixel after first triangle
    const uint8_t* pixels = dev.GetSwapchainPixels();
    size_t ci = (4 * 8 + 4) * 4;
    uint8_t r1 = pixels[ci], g1 = pixels[ci+1], b1 = pixels[ci+2];

    // Draw triangle 2 (farther, z=0.8) - should be depth-rejected
    dev.BindVertexBuffer(vbo2, 0, 0);
    dev.Draw(3);

    // Center pixel should not change (depth test rejects farther triangle)
    uint8_t r2 = pixels[ci], g2 = pixels[ci+1], b2 = pixels[ci+2];
    TEST_ASSERT_EQUAL_UINT8(r1, r2);
    TEST_ASSERT_EQUAL_UINT8(g1, g2);
    TEST_ASSERT_EQUAL_UINT8(b1, b2);

    dev.EndRenderPass();
    dev.EndFrame();

    dev.DestroyBuffer(vbo1);
    dev.DestroyBuffer(vbo2);
    dev.DestroyBuffer(ubo);
    dev.DestroyPipeline(pip);
    dev.Shutdown();
}

void TestLineRasterWritesPixels() {
    SoftwareRHIDevice dev;
    dev.Initialize();
    dev.OnResize(16, 16);

    // Create line-topology pipeline, 28-byte stride (DebugLineVertex)
    RHIPipelineDesc pipDesc;
    pipDesc.topology = RHITopology::LineList;
    pipDesc.vertexStride = 28;
    pipDesc.vertexAttrCount = 2;
    pipDesc.depthStencil.depthTest = false;
    pipDesc.depthStencil.depthWrite = false;
    pipDesc.rasterizer.cullMode = RHICullMode::None;
    RHIPipeline pip = dev.CreatePipeline(pipDesc);

    // UBO with identity matrices
    float uboData[84] = {};
    WriteIdentityMatrix(uboData);
    WriteIdentityMatrix(uboData + 16);
    WriteIdentityMatrix(uboData + 32);
    RHIBufferDesc uboDesc;
    uboDesc.size = sizeof(uboData);
    RHIBuffer ubo = dev.CreateBuffer(uboDesc);
    dev.UpdateBuffer(ubo, uboData, sizeof(uboData), 0);

    // One line segment: horizontal through the center
    // DebugLineVertex: pos(3f) + color(4f) = 28 bytes
    struct DebugLineVertex { float px,py,pz, r,g,b,a; };
    DebugLineVertex line[2] = {
        {-0.5f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f, 1.0f},  // red
        { 0.5f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f, 1.0f},   // red
    };

    RHIBufferDesc bufDesc;
    bufDesc.size = sizeof(line);
    RHIBuffer vbo = dev.CreateBuffer(bufDesc);
    dev.UpdateBuffer(vbo, line, sizeof(line), 0);

    // Clear to black
    RHIClearValue clear;
    clear.color[0] = 0; clear.color[1] = 0;
    clear.color[2] = 0; clear.color[3] = 1.0f;

    dev.BeginFrame();
    dev.BeginSwapchainRenderPass(clear);
    dev.BindPipeline(pip);
    dev.BindVertexBuffer(vbo, 0, 0);
    dev.BindUniformBuffer(ubo, 0, 0, 0, sizeof(uboData));
    RHIViewport vp = {0, 0, 16, 16, 0, 1};
    dev.SetViewport(vp);

    dev.Draw(2);
    dev.EndRenderPass();
    dev.EndFrame();

    // Verify: center pixel should be red
    const uint8_t* pixels = dev.GetSwapchainPixels();
    size_t ci = (8 * 16 + 8) * 4; // center of 16x16
    TEST_ASSERT_EQUAL_UINT8(255, pixels[ci]);   // R
    TEST_ASSERT_EQUAL_UINT8(0,   pixels[ci+1]); // G
    TEST_ASSERT_EQUAL_UINT8(0,   pixels[ci+2]); // B

    dev.DestroyBuffer(vbo);
    dev.DestroyBuffer(ubo);
    dev.DestroyPipeline(pip);
    dev.Shutdown();
}

void TestBlitFullscreenQuad() {
    SoftwareRHIDevice dev;
    dev.Initialize();
    dev.OnResize(4, 4);

    // Create blit pipeline: 2 attrs, 16 stride, no depth
    RHIPipelineDesc pipDesc;
    pipDesc.topology = RHITopology::TriangleList;
    pipDesc.vertexStride = 16;
    pipDesc.vertexAttrCount = 2;
    pipDesc.depthStencil.depthTest = false;
    pipDesc.depthStencil.depthWrite = false;
    RHIPipeline pip = dev.CreatePipeline(pipDesc);

    // Create a 2x2 source texture (RGBA8) - solid blue
    RHITextureDesc texDesc;
    texDesc.width = 2; texDesc.height = 2;
    texDesc.format = RHIFormat::RGBA8_Unorm;
    RHITexture srcTex = dev.CreateTexture(texDesc);
    uint8_t blue[16] = {
        0,0,255,255,  0,0,255,255,
        0,0,255,255,  0,0,255,255
    };
    dev.UpdateTexture(srcTex, blue, sizeof(blue), 2, 2);

    // Clear swapchain to black
    RHIClearValue clear;
    clear.color[0] = 0; clear.color[1] = 0;
    clear.color[2] = 0; clear.color[3] = 1.0f;

    dev.BeginFrame();
    dev.BeginSwapchainRenderPass(clear);
    dev.BindPipeline(pip);
    dev.BindTexture(srcTex, 0, 0); // Blit samples from set=0, binding=0

    // Dummy vertex buffer (blit ignores vertex data)
    struct FullscreenVert { float px,py, u,v; };
    FullscreenVert verts[6] = {};
    RHIBufferDesc bufDesc;
    bufDesc.size = sizeof(verts);
    RHIBuffer vbo = dev.CreateBuffer(bufDesc);
    dev.UpdateBuffer(vbo, verts, sizeof(verts), 0);
    dev.BindVertexBuffer(vbo, 0, 0);

    RHIViewport vp = {0, 0, 4, 4, 0, 1};
    dev.SetViewport(vp);

    dev.Draw(6);
    dev.EndRenderPass();
    dev.EndFrame();

    // Verify: all pixels should be blue (blit copies source texture)
    const uint8_t* pixels = dev.GetSwapchainPixels();
    for (int i = 0; i < 4 * 4; ++i) {
        size_t pi = (size_t)i * 4;
        TEST_ASSERT_EQUAL_UINT8(0,   pixels[pi]);     // R
        TEST_ASSERT_EQUAL_UINT8(0,   pixels[pi + 1]); // G
        TEST_ASSERT_EQUAL_UINT8(255, pixels[pi + 2]); // B
        TEST_ASSERT_EQUAL_UINT8(255, pixels[pi + 3]); // A
    }

    dev.DestroyBuffer(vbo);
    dev.DestroyTexture(srcTex);
    dev.DestroyPipeline(pip);
    dev.Shutdown();
}

// -- Runner ----------------------------------------------------------

void RunAllTests() {
    RUN_TEST(TestInitializeAndShutdown);
    RUN_TEST(TestGetNameAndCaps);
    RUN_TEST(TestCreateDestroyBuffer);
    RUN_TEST(TestUpdateAndMapBuffer);
    RUN_TEST(TestCreateDestroyTexture);
    RUN_TEST(TestUpdateTexture);
    RUN_TEST(TestCreateDestroyShader);
    RUN_TEST(TestCreateDestroyPipeline);
    RUN_TEST(TestCreateDestroyRenderPass);
    RUN_TEST(TestCreateDestroyFramebuffer);
    RUN_TEST(TestSwapchainRenderPass);
    RUN_TEST(TestOnResizeAllocatesSwapchain);
    RUN_TEST(TestBeginRenderPassClearsColor);
    RUN_TEST(TestBeginSwapchainRenderPassClears);
    RUN_TEST(TestBindStateTracking);
    RUN_TEST(TestDrawIsNoOp);
    RUN_TEST(TestTriangleRasterWritesPixels);
    RUN_TEST(TestTriangleDepthTest);
    RUN_TEST(TestLineRasterWritesPixels);
    RUN_TEST(TestBlitFullscreenQuad);
}

} // namespace TestSoftwareRHI
