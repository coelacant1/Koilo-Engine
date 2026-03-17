// UI vertex shader for Vulkan batched rendering.
// Converts pixel coordinates to NDC using a push constant viewport size.
#version 450

layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec4 a_color;
layout(location = 3) in vec4 a_sdf;   // (halfW, halfH, borderWidth, 0)
layout(location = 4) in vec4 a_radii; // (TL, TR, BR, BL)

layout(push_constant) uniform PushConstants {
    vec2 u_viewport;
    int u_useTexture;
};

layout(location = 0) out vec2 v_uv;
layout(location = 1) out vec4 v_color;
layout(location = 2) out vec4 v_sdf;
layout(location = 3) out vec4 v_radii;

void main() {
    vec2 ndc = (a_position / u_viewport) * 2.0 - 1.0;
    gl_Position = vec4(ndc, 0.0, 1.0);
    v_uv = a_uv;
    v_color = a_color;
    v_sdf = a_sdf;
    v_radii = a_radii;
}
