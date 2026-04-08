// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file led_gather_compute.cpp
 * @brief GPU compute shader implementation for LED pixel gathering.
 */

#ifdef KL_HAVE_OPENGL_BACKEND

#include <koilo/systems/display/led/led_gather_compute.hpp>

#include <glad/glad.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <string>

namespace koilo {

// Embedded compute shader source (GL 4.3 / #version 430)
static const char* const kGatherShaderSource = R"GLSL(
#version 430
layout(local_size_x = 64) in;

// Scene FBO bound as a sampler (read-only texture access with filtering).
layout(binding = 0) uniform sampler2D sceneFBO;

// LED positions as normalized UV pairs [u0, v0, u1, v1, ...].
layout(std430, binding = 1) readonly buffer LEDPositions {
    vec2 positions[];
};

// Output: one uint per LED, packed as R | (G << 8) | (B << 16).
layout(std430, binding = 2) writeonly buffer LEDColors {
    uint packedRGB[];
};

uniform float uGamma;
uniform float uBrightness;
uniform uint  uPixelCount;

void main() {
    uint idx = gl_GlobalInvocationID.x;
    if (idx >= uPixelCount) return;

    vec2 uv = positions[idx];

    // Out-of-range pixels produce black.
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        packedRGB[idx] = 0u;
        return;
    }

    // Sample the FBO (hardware bilinear filtering via sampler2D).
    vec3 color = texture(sceneFBO, uv).rgb;

    // Gamma correction + brightness.
    color = pow(color, vec3(uGamma)) * uBrightness;

    // Pack to RGB888.
    uvec3 rgb = uvec3(clamp(color * 255.0, 0.0, 255.0));
    packedRGB[idx] = rgb.r | (rgb.g << 8u) | (rgb.b << 16u);
}
)GLSL";

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

LEDGatherCompute::LEDGatherCompute() = default;

LEDGatherCompute::~LEDGatherCompute() {
    Shutdown();
}

const char* LEDGatherCompute::GetShaderSource() {
    return kGatherShaderSource;
}

bool LEDGatherCompute::IsSupported() {
    // GL 4.3 core includes compute shaders.
    // Also accept the ARB extension on earlier contexts.
    return GLAD_GL_ARB_compute_shader || (GLVersion.major > 4) ||
           (GLVersion.major == 4 && GLVersion.minor >= 3);
}

bool LEDGatherCompute::Initialize() {
    if (ready_) return true;
    if (!IsSupported()) return false;

    // Compile compute shader.
    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    const char* src = kGatherShaderSource;
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint status = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status) {
        char log[512];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::cerr << "[LEDGatherCompute] Shader compile error: " << log << "\n";
        glDeleteShader(shader);
        return false;
    }

    // Link program.
    program_ = glCreateProgram();
    glAttachShader(program_, shader);
    glLinkProgram(program_);
    glDeleteShader(shader);

    glGetProgramiv(program_, GL_LINK_STATUS, &status);
    if (!status) {
        char log[512];
        glGetProgramInfoLog(program_, sizeof(log), nullptr, log);
        std::cerr << "[LEDGatherCompute] Program link error: " << log << "\n";
        glDeleteProgram(program_);
        program_ = 0;
        return false;
    }

    gammaLoc_      = glGetUniformLocation(program_, "uGamma");
    brightnessLoc_ = glGetUniformLocation(program_, "uBrightness");
    pixelCountLoc_ = glGetUniformLocation(program_, "uPixelCount");

    // Create SSBOs (initially empty, sized on UploadPositions).
    glGenBuffers(1, &positionsSSBO_);
    glGenBuffers(1, &outputSSBO_);

    ready_ = true;
    return true;
}

void LEDGatherCompute::Shutdown() {
    if (positionsSSBO_) { glDeleteBuffers(1, &positionsSSBO_); positionsSSBO_ = 0; }
    if (outputSSBO_)    { glDeleteBuffers(1, &outputSSBO_);    outputSSBO_ = 0; }
    if (program_)       { glDeleteProgram(program_);           program_ = 0; }
    pixelCount_ = 0;
    ready_ = false;
}

void LEDGatherCompute::UploadPositions(const float* positions, uint32_t pixelCount) {
    if (!ready_ || !positions || pixelCount == 0) return;

    pixelCount_ = pixelCount;

    // Positions SSBO: pixelCount * 2 floats (vec2 per LED).
    const GLsizeiptr posByteSize = static_cast<GLsizeiptr>(pixelCount) * 2 * sizeof(float);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, positionsSSBO_);
    glBufferData(GL_SHADER_STORAGE_BUFFER, posByteSize, positions, GL_STATIC_DRAW);

    // Output SSBO: pixelCount uints (packed RGB per LED).
    const GLsizeiptr outByteSize = static_cast<GLsizeiptr>(pixelCount) * sizeof(uint32_t);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, outputSSBO_);
    glBufferData(GL_SHADER_STORAGE_BUFFER, outByteSize, nullptr, GL_DYNAMIC_READ);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void LEDGatherCompute::Dispatch(uint32_t fboTextureId, float gamma, float brightness) {
    if (!ready_ || pixelCount_ == 0) return;

    glUseProgram(program_);

    // Bind FBO texture to sampler unit 0.
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fboTextureId);

    // Set uniforms.
    if (gammaLoc_ >= 0)      glUniform1f(gammaLoc_, gamma);
    if (brightnessLoc_ >= 0) glUniform1f(brightnessLoc_, brightness);
    if (pixelCountLoc_ >= 0) glUniform1ui(pixelCountLoc_, pixelCount_);

    // Bind SSBOs.
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, positionsSSBO_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, outputSSBO_);

    // Dispatch: ceil(pixelCount / workGroupSize) workgroups.
    const uint32_t numGroups = (pixelCount_ + kWorkGroupSize - 1) / kWorkGroupSize;
    glDispatchCompute(numGroups, 1, 1);

    // Barrier: ensure SSBO writes are visible before ReadBack.
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glUseProgram(0);
}

size_t LEDGatherCompute::ReadBack(uint8_t* output) {
    if (!ready_ || pixelCount_ == 0 || !output) return 0;

    const size_t outputBytes = static_cast<size_t>(pixelCount_) * 3;

    // Map the output SSBO and unpack uint -> RGB888.
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, outputSSBO_);
    const void* mapped = glMapBufferRange(
        GL_SHADER_STORAGE_BUFFER, 0,
        static_cast<GLsizeiptr>(pixelCount_) * sizeof(uint32_t),
        GL_MAP_READ_BIT);

    if (!mapped) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        return 0;
    }

    const uint32_t* packed = static_cast<const uint32_t*>(mapped);
    for (uint32_t i = 0; i < pixelCount_; ++i) {
        const uint32_t p = packed[i];
        output[i * 3 + 0] = static_cast<uint8_t>(p & 0xFF);
        output[i * 3 + 1] = static_cast<uint8_t>((p >> 8) & 0xFF);
        output[i * 3 + 2] = static_cast<uint8_t>((p >> 16) & 0xFF);
    }

    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    return outputBytes;
}

} // namespace koilo

#endif // KL_HAVE_OPENGL_BACKEND
