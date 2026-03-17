// Shared Vulkan vertex shader for all KSL scene geometry.
// Matches the layout expected by KSL fragment shaders in Vulkan mode.
#version 450

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_uv;

layout(set = 0, binding = 0) uniform TransformUBO {
    mat4 u_model;
    mat4 u_view;
    mat4 u_projection;
    vec3 u_cameraPos;
};

layout(location = 0) out vec3 v_position;
layout(location = 1) out vec3 v_normal;
layout(location = 2) out vec2 v_uv;
layout(location = 3) out vec3 v_viewDir;

void main() {
    vec4 worldPos = u_model * vec4(a_position, 1.0);
    v_position = worldPos.xyz;
    v_normal = normalize(mat3(u_model) * a_normal);
    v_uv = a_uv;
    v_viewDir = normalize(worldPos.xyz - u_cameraPos);
    gl_Position = u_projection * u_view * worldPos;
}
