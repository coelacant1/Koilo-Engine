// UI fragment shader for Vulkan batched rendering.
// Supports solid color, textured (font atlas), and SDF rounded rects.
#version 450

layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec4 v_color;
layout(location = 2) in vec4 v_sdf;   // (halfW, halfH, borderWidth, 0)
layout(location = 3) in vec4 v_radii; // (TL, TR, BR, BL)

layout(push_constant) uniform PushConstants {
    vec2 u_viewport;
    int u_useTexture;
};

layout(set = 0, binding = 0) uniform sampler2D u_texture;

layout(location = 0) out vec4 FragColor;

float roundedRectSDF(vec2 p, vec2 b, vec4 r) {
    float cr = (p.x > 0.0)
        ? ((p.y > 0.0) ? r.z : r.y)
        : ((p.y > 0.0) ? r.w : r.x);
    vec2 q = abs(p) - b + vec2(cr);
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - cr;
}

void main() {
    float halfW = v_sdf.x;
    float halfH = v_sdf.y;
    float borderW = v_sdf.z;
    bool hasRadius = (v_radii.x > 0.0 || v_radii.y > 0.0 || v_radii.z > 0.0 || v_radii.w > 0.0);

    if (halfW > 0.0 && hasRadius) {
        vec2 halfSize = vec2(halfW, halfH);
        vec2 p = v_uv * halfSize * 2.0 - halfSize;

        float outerD = roundedRectSDF(p, halfSize, v_radii);
        float outerAlpha = 1.0 - smoothstep(-0.75, 0.75, outerD);

        if (borderW > 0.0) {
            vec4 innerR = max(v_radii - vec4(borderW), vec4(0.0));
            vec2 innerHalf = halfSize - vec2(borderW);
            float innerD = roundedRectSDF(p, innerHalf, innerR);
            float innerAlpha = 1.0 - smoothstep(-0.75, 0.75, innerD);
            FragColor = vec4(v_color.rgb, v_color.a * (outerAlpha - innerAlpha));
        } else {
            FragColor = vec4(v_color.rgb, v_color.a * outerAlpha);
        }
    } else if (u_useTexture != 0) {
        float alpha = texture(u_texture, v_uv).r;
        FragColor = vec4(v_color.rgb, v_color.a * alpha);
    } else {
        FragColor = v_color;
    }
}
