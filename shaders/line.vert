#version 450

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec4 a_color;

layout(set = 0, binding = 0) uniform TransformUBO {
    mat4 u_model;
    mat4 u_view;
    mat4 u_projection;
    vec4 u_cameraPos;
    mat4 u_inverseViewProj;
};

layout(location = 0) out vec4 v_color;

void main() {
    v_color = a_color;
    gl_Position = u_projection * u_view * u_model * vec4(a_position, 1.0);
}
