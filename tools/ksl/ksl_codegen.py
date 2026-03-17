#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""
KSL Codegen - Converts .ksl.hpp shader files to GLSL fragment shader source.

Reads KSL_PARAM macros via regex to generate uniform declarations.
Extracts shade() method body via brace matching.
Applies text substitution to convert ksl:: C++ -> GLSL.

Usage: python3 ksl_codegen.py input.ksl.hpp [-o output.glsl] [--vertex]
"""

import re
import sys
import argparse
from pathlib import Path

# --- Text substitution rules ---

# ksl::ShadeInput field -> GLSL varying/uniform mapping
INPUT_MAP = {
    r'in\.position':    'v_position',
    r'in\.normal':      'v_normal',
    r'in\.uv':          'v_uv',
    r'in\.viewDir':     'v_viewDir',
    r'in\.depth':       'v_depth',
    r'in\.ctx->time':        'u_time',
    r'in\.ctx->dt':          'u_dt',
    r'in\.ctx->frameCount':  'u_frameCount',
    r'in\.ctx->lightCount':  'u_lightCount',
    r'in\.ctx->textureCount':'u_textureCount',
    r'in\.ctx->sampleCount': 'u_sampleCount',
    r'in\.ctx->spectrumCount':'u_spectrumCount',
}

# Light array access: in.ctx->lights[i].field -> u_lights[i].field
LIGHT_PATTERN = re.compile(r'in\.ctx->lights\[([^\]]+)\]\.(\w+)')

# Texture sampling: ksl::sample(in.ctx->textures[N], uv) -> texture(u_textureN, uv)
SAMPLE_PATTERN = re.compile(r'ksl::sample\(in\.ctx->textures\[(\d+)\],\s*([^)]+)\)')

# Audio arrays: in.ctx->audioSamples[i] -> u_audioSamples[i]
AUDIO_SAMPLE_PATTERN = re.compile(r'in\.ctx->audioSamples\[([^\]]+)\]')
AUDIO_SPECTRUM_PATTERN = re.compile(r'in\.ctx->audioSpectrum\[([^\]]+)\]')

# C++ -> GLSL type/syntax
TYPE_REPLACEMENTS = [
    (r'ksl::vec4',      'vec4'),
    (r'ksl::vec3',      'vec3'),
    (r'ksl::vec2',      'vec2'),
    (r'ksl::',          ''),        # Strip remaining ksl:: namespace
    (r'(\d+)\.(\d+)f',  r'\1.\2'),  # 1.0f -> 1.0
    (r'(\d+)f\b',       r'\1.0'),   # 3f -> 3.0
    (r'static_cast<int>',  'int'),
    (r'static_cast<float>','float'),
    (r'const\s+',       ''),        # Strip const qualifiers
]

# Swizzle: .xy() -> .xy, .xyz() -> .xyz, .rgb() -> .rgb
SWIZZLE_PATTERN = re.compile(r'\.(xy|xz|yz|xyz|rgb)\(\)')


def extract_params(source):
    """Extract KSL_PARAM macro invocations -> list of (type, name, array_size or None).
    Returns params in declaration order (matching C++ struct layout)."""
    entries = []
    # KSL_PARAM(TYPE, NAME, DESC, MIN, MAX)
    for m in re.finditer(r'KSL_PARAM\(\s*([\w:]+)\s*,\s*(\w+)\s*,', source):
        typ = m.group(1).replace('ksl::', '')
        entries.append((m.start(), typ, m.group(2), None))
    # KSL_PARAM_ARRAY(TYPE, NAME, SIZE, DESC)
    for m in re.finditer(r'KSL_PARAM_ARRAY\(\s*([\w:]+)\s*,\s*(\w+)\s*,\s*(\d+)\s*,', source):
        typ = m.group(1).replace('ksl::', '')
        entries.append((m.start(), typ, m.group(2), int(m.group(3))))
    # Sort by source position to preserve declaration order
    entries.sort(key=lambda e: e[0])
    return [(typ, name, arr) for _, typ, name, arr in entries]


def extract_shade_body(source):
    """Extract the body of the shade() method via brace matching."""
    # Find the shade method signature
    pattern = re.compile(
        r'(?:ksl::)?vec4\s+shade\s*\(\s*(?:const\s+)?(?:ksl::)?ShadeInput\s*&\s*(\w+)\s*\)\s*const\s*(?:override\s*)?\{'
    )
    m = pattern.search(source)
    if not m:
        raise ValueError("Could not find shade() method signature")

    input_var = m.group(1)  # The name of the ShadeInput parameter (usually "in")
    start = m.end()  # Position right after opening brace

    # Brace matching
    depth = 1
    pos = start
    while pos < len(source) and depth > 0:
        if source[pos] == '{':
            depth += 1
        elif source[pos] == '}':
            depth -= 1
        pos += 1

    body = source[start:pos - 1].strip()
    return body, input_var


def extract_helper_functions(source):
    """Extract any helper functions defined in the struct before shade().
    These are non-virtual methods that aren't shade(), Params(), constructors, etc."""
    helpers = []
    # Find functions that look like: RetType funcName(...) const { ... }
    # but exclude shade, Params, operator, constructors
    skip = {'shade', 'Params'}

    # Find the struct body
    struct_match = re.search(r'struct\s+\w+\s*:\s*public\s+ksl::Shader\s*\{', source)
    if not struct_match:
        return helpers

    struct_start = struct_match.end()

    # Find shade method position to only look before it
    shade_match = re.search(r'vec4\s+shade\s*\(', source[struct_start:])
    if not shade_match:
        return helpers

    pre_shade = source[struct_start:struct_start + shade_match.start()]

    # Look for function definitions (very simple heuristic)
    func_pattern = re.compile(
        r'((?:inline\s+)?(?:ksl::)?(?:vec[234]|float|int|void)\s+(\w+)\s*\([^)]*\)\s*(?:const\s*)?\{)'
    )

    for fm in func_pattern.finditer(pre_shade):
        fname = fm.group(2)
        if fname in skip:
            continue
        # Extract body via brace matching
        func_start = struct_start + fm.start()
        sig_end = struct_start + fm.end()
        depth = 1
        pos = sig_end
        while pos < len(source) and depth > 0:
            if source[pos] == '{':
                depth += 1
            elif source[pos] == '}':
                depth -= 1
            pos += 1
        func_text = source[func_start:pos]
        helpers.append(func_text)

    return helpers


def transform_body(body, input_var, params):
    """Apply text substitutions to convert C++ shade body -> GLSL."""
    result = body

    # Replace input variable references
    for cpp_pat, glsl_name in INPUT_MAP.items():
        result = re.sub(cpp_pat.replace('in.', re.escape(input_var) + r'\.'), glsl_name, result)

    # Light array access
    result = LIGHT_PATTERN.sub(lambda m: f'u_lights[{m.group(1)}].{m.group(2)}', result)

    # Texture sampling
    result = SAMPLE_PATTERN.sub(lambda m: f'texture(u_texture{m.group(1)}, {m.group(2)})', result)

    # Audio arrays
    result = AUDIO_SAMPLE_PATTERN.sub(lambda m: f'u_audioSamples[{m.group(1)}]', result)
    result = AUDIO_SPECTRUM_PATTERN.sub(lambda m: f'u_audioSpectrum[{m.group(1)}]', result)

    # Swizzle methods -> GLSL swizzle
    result = SWIZZLE_PATTERN.sub(r'.\1', result)

    # Replace member field access (shader params) with uniform names
    for typ, name, arr_size in params:
        # Only replace bare identifiers, not part of other words
        result = re.sub(r'\b' + name + r'\b', f'u_{name}', result)

    # C++ -> GLSL type/syntax replacements
    for pattern, replacement in TYPE_REPLACEMENTS:
        result = re.sub(pattern, replacement, result)

    # C++ constructor-style variable init -> GLSL assignment
    # e.g. "vec3 Lo(0.0)" -> "vec3 Lo = vec3(0.0)"
    # e.g. "vec2 uv(a,\n         b)" -> "vec2 uv = vec2(a,\n         b)"
    # Uses brace matching for parentheses to handle multi-line
    def replace_ctor_init(text):
        out = []
        i = 0
        ctor_re = re.compile(r'\b(vec[234]|float|int)\s+(\w+)\(')
        while i < len(text):
            m = ctor_re.search(text, i)
            if not m:
                out.append(text[i:])
                break
            out.append(text[i:m.start()])
            typ = m.group(1)
            var = m.group(2)
            # Find matching close paren
            paren_start = m.end() - 1  # position of '('
            depth = 1
            j = paren_start + 1
            while j < len(text) and depth > 0:
                if text[j] == '(':
                    depth += 1
                elif text[j] == ')':
                    depth -= 1
                j += 1
            args = text[paren_start + 1:j - 1]
            out.append(f'{typ} {var} = {typ}({args})')
            i = j
        return ''.join(out)

    result = replace_ctor_init(result)

    return result


def transform_helper(func_text, input_var, params):
    """Transform a helper function from C++ to GLSL."""
    result = func_text

    # Remove 'inline', 'const' qualifiers, 'ksl::' prefix
    result = re.sub(r'\binline\s+', '', result)
    result = SWIZZLE_PATTERN.sub(r'.\1', result)

    for typ, name, arr_size in params:
        result = re.sub(r'\b' + name + r'\b', f'u_{name}', result)

    for pattern, replacement in TYPE_REPLACEMENTS:
        result = re.sub(pattern, replacement, result)

    return result


def generate_uniforms(params):
    """Generate GLSL uniform declarations from params."""
    lines = []
    for typ, name, arr_size in params:
        glsl_type = typ.replace('ksl::', '')
        if arr_size:
            lines.append(f'uniform {glsl_type} u_{name}[{arr_size}];')
        else:
            lines.append(f'uniform {glsl_type} u_{name};')
    return '\n'.join(lines)


# --- KSL Standard Library (non-builtin functions emitted into GLSL) ---

KSL_STDLIB = {
    'map': '''\
float map(float x, float in_min, float in_max, float out_min, float out_max) {
    return out_min + (out_max - out_min) * (x - in_min) / (in_max - in_min);
}''',
    'cosineInterpolation': '''\
float cosineInterpolation(float a, float b, float t) {
    float t2 = (1.0 - cos(t * 3.14159265)) / 2.0;
    return a * (1.0 - t2) + b * t2;
}''',
    'rotate2d': '''\
vec2 rotate2d(vec2 p, float angleDeg, vec2 center) {
    float rad = angleDeg * 3.14159265 / 180.0;
    float c = cos(rad);
    float s = sin(rad);
    vec2 d = p - center;
    return vec2(d.x * c - d.y * s + center.x, d.x * s + d.y * c + center.y);
}''',
    'hueShift': '''\
vec3 hueShift(vec3 c, float angleDeg) {
    float rad = angleDeg * 3.14159265 / 180.0;
    float cosA = cos(rad);
    float sinA = sin(rad);
    float k = (1.0 - cosA) / 3.0;
    float sq = sinA * 0.57735026919;
    return vec3(
        c.x * (cosA + k) + c.y * (k - sq) + c.z * (k + sq),
        c.x * (k + sq) + c.y * (cosA + k) + c.z * (k - sq),
        c.x * (k - sq) + c.y * (k + sq) + c.z * (cosA + k)
    );
}''',
    'hash': '''\
float hash(float n) {
    return fract(sin(n) * 43758.5453123);
}''',
    'noise3d': '''\
float noise3d(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    vec3 u = f * f * (3.0 - 2.0 * f);
    float n = i.x + i.y * 157.0 + i.z * 113.0;
    return mix(mix(mix(hash(n + 0.0),   hash(n + 1.0),   u.x),
                   mix(hash(n + 157.0), hash(n + 158.0), u.x), u.y),
               mix(mix(hash(n + 113.0), hash(n + 114.0), u.x),
                   mix(hash(n + 270.0), hash(n + 271.0), u.x), u.y), u.z) * 2.0 - 1.0;
}''',
}

# Dependencies between stdlib functions
KSL_STDLIB_DEPS = {
    'noise3d': ['hash'],
}


def detect_stdlib_usage(source):
    """Detect which KSL stdlib functions are used in the source."""
    used = set()
    for name in KSL_STDLIB:
        if f'ksl::{name}' in source:
            used.add(name)
    # Add dependencies
    added = True
    while added:
        added = False
        for name in list(used):
            for dep in KSL_STDLIB_DEPS.get(name, []):
                if dep not in used:
                    used.add(dep)
                    added = True
    # Return in dependency order (deps first)
    ordered = []
    remaining = set(used)
    while remaining:
        for name in list(remaining):
            deps = set(KSL_STDLIB_DEPS.get(name, []))
            if deps.issubset(set(ordered)):
                ordered.append(name)
                remaining.remove(name)
    return ordered


def detect_features(source, body):
    """Detect which extended features the shader uses."""
    features = {
        'lights': bool(re.search(r'in\.ctx->lights|in\.ctx->lightCount', source)),
        'textures': bool(re.search(r'in\.ctx->textures|ksl::sample', source)),
        'audio_samples': bool(re.search(r'in\.ctx->audioSamples|in\.ctx->sampleCount', source)),
        'audio_spectrum': bool(re.search(r'in\.ctx->audioSpectrum|in\.ctx->spectrumCount', source)),
        'time': bool(re.search(r'in\.ctx->time|in\.ctx->dt|in\.ctx->frameCount', source)),
        'depth': bool(re.search(r'in\.depth', source)),
    }
    return features


def generate_glsl(source, shader_name=None, vulkan=False):
    """Generate complete GLSL fragment shader from .ksl.hpp source.

    Args:
        source: .ksl.hpp file contents.
        shader_name: override for comment header.
        vulkan: if True, emit #version 450 with explicit layout qualifiers
                and descriptor set bindings (Vulkan/SPIR-V compatible).
    """
    params = extract_params(source)
    body, input_var = extract_shade_body(source)
    helpers = extract_helper_functions(source)
    features = detect_features(source, body)
    stdlib_funcs = detect_stdlib_usage(source)

    if not shader_name:
        m = re.search(r'struct\s+(KSL_\w+|[\w]+Shader)\s*:', source)
        shader_name = m.group(1) if m else 'UnknownShader'

    lines = []
    lines.append(f'// Auto-generated from {shader_name}.ksl.hpp - DO NOT EDIT')

    if vulkan:
        lines.append('#version 450')
    else:
        lines.append('#version 330 core')
    lines.append('')

    # Varyings (always present) - Vulkan needs explicit locations
    if vulkan:
        lines.append('layout(location = 0) in vec3 v_position;')
        lines.append('layout(location = 1) in vec3 v_normal;')
        lines.append('layout(location = 2) in vec2 v_uv;')
        lines.append('layout(location = 3) in vec3 v_viewDir;')
        next_in_loc = 4
        if features['depth']:
            lines.append(f'layout(location = {next_in_loc}) in float v_depth;')
    else:
        lines.append('in vec3 v_position;')
        lines.append('in vec3 v_normal;')
        lines.append('in vec2 v_uv;')
        lines.append('in vec3 v_viewDir;')
        if features['depth']:
            lines.append('in float v_depth;')

    lines.append('')

    if vulkan:
        lines.append('layout(location = 0) out vec4 fragColor;')
    else:
        lines.append('out vec4 fragColor;')
    lines.append('')

    # --- Descriptor set layout for Vulkan ---
    # Set 0: Scene UBO (transforms, time, camera, lights)
    # Set 1: Material UBO (shader-specific params)
    # Set 2: Texture samplers
    scene_ubo_members = []
    material_ubo_members = []
    next_sampler_binding = 0

    if vulkan:
        # Build scene UBO members
        if features['time']:
            scene_ubo_members.append('    float u_time;')
            scene_ubo_members.append('    float u_dt;')
            scene_ubo_members.append('    int u_frameCount;')
            scene_ubo_members.append('    int _pad_scene0;')
        if features['lights']:
            scene_ubo_members.append('    int u_lightCount;')
            scene_ubo_members.append('    int _pad_scene1;')
            scene_ubo_members.append('    int _pad_scene2;')
            scene_ubo_members.append('    int _pad_scene3;')

        # Scene UBO
        if scene_ubo_members:
            lines.append('layout(set = 0, binding = 2) uniform SceneUBO {')
            for m in scene_ubo_members:
                lines.append(m)
            lines.append('};')
            lines.append('')

        # Light SSBO (set 0, binding 1) - use SSBO for struct array
        if features['lights']:
            lines.append('#define MAX_LIGHTS 16')
            lines.append('struct Light {')
            lines.append('    vec3 position;')
            lines.append('    float intensity;')
            lines.append('    vec3 color;')
            lines.append('    float falloff;')
            lines.append('    float curve;')
            lines.append('    float _pad0;')
            lines.append('    float _pad1;')
            lines.append('    float _pad2;')
            lines.append('};')
            lines.append('layout(set = 0, binding = 1, std430) readonly buffer LightBuffer {')
            lines.append('    Light u_lights[];')
            lines.append('};')
            lines.append('')

        # Audio SSBO (set 0, binding 3 + 4) - binding 2 is SceneUBO
        if features['audio_samples']:
            lines.append('#define MAX_AUDIO_SAMPLES 1024')
            lines.append('layout(set = 0, binding = 3, std430) readonly buffer AudioSampleBuffer {')
            lines.append('    float u_audioSamples[];')
            lines.append('};')
            # sampleCount is in scene UBO - add it there
            lines.append('')
        if features['audio_spectrum']:
            lines.append('#define MAX_AUDIO_SPECTRUM 512')
            lines.append('layout(set = 0, binding = 4, std430) readonly buffer AudioSpectrumBuffer {')
            lines.append('    float u_audioSpectrum[];')
            lines.append('};')
            lines.append('')

        # Audio count uniforms (push constant or scene UBO extension)
        # For simplicity, emit as part of scene UBO above or as standalone
        if features['audio_samples'] and 'int u_sampleCount;' not in '\n'.join(scene_ubo_members):
            pass  # sampleCount will be in push constants
        if features['audio_spectrum'] and 'int u_spectrumCount;' not in '\n'.join(scene_ubo_members):
            pass  # spectrumCount will be in push constants

        # Material UBO (set 1, binding 0) - shader-specific params
        for typ, name, arr_size in params:
            glsl_type = typ.replace('ksl::', '')
            if arr_size:
                material_ubo_members.append(f'    {glsl_type} u_{name}[{arr_size}];')
            else:
                material_ubo_members.append(f'    {glsl_type} u_{name};')

        if material_ubo_members:
            lines.append('layout(set = 1, binding = 0) uniform MaterialUBO {')
            for m in material_ubo_members:
                lines.append(m)
            lines.append('};')
            lines.append('')

        # Push constants for audio counts and shader ID
        push_members = []
        if features['audio_samples']:
            push_members.append('    int u_sampleCount;')
        if features['audio_spectrum']:
            push_members.append('    int u_spectrumCount;')
        if push_members:
            lines.append('layout(push_constant) uniform PushConstants {')
            for m in push_members:
                lines.append(m)
            lines.append('};')
            lines.append('')

        # Texture samplers (set 2)
        if features['textures']:
            for i in range(8):
                if f'in.ctx->textures[{i}]' in source or f'u_texture{i}' in source:
                    lines.append(f'layout(set = 2, binding = {next_sampler_binding}) uniform sampler2D u_texture{i};')
                    next_sampler_binding += 1
            lines.append('')

    else:
        # OpenGL: plain uniforms (original path)
        if features['time']:
            lines.append('uniform float u_time;')
            lines.append('uniform float u_dt;')
            lines.append('uniform int u_frameCount;')
            lines.append('')

        if features['lights']:
            lines.append('#define MAX_LIGHTS 16')
            lines.append('struct Light {')
            lines.append('    vec3 position;')
            lines.append('    vec3 color;')
            lines.append('    float intensity;')
            lines.append('    float falloff;')
            lines.append('    float curve;')
            lines.append('};')
            lines.append('uniform Light u_lights[MAX_LIGHTS];')
            lines.append('uniform int u_lightCount;')
            lines.append('')

        if features['textures']:
            for i in range(8):
                if f'in.ctx->textures[{i}]' in source or f'u_texture{i}' in source:
                    lines.append(f'uniform sampler2D u_texture{i};')
            lines.append('')

        if features['audio_samples']:
            lines.append('#define MAX_AUDIO_SAMPLES 1024')
            lines.append('uniform float u_audioSamples[MAX_AUDIO_SAMPLES];')
            lines.append('uniform int u_sampleCount;')
            lines.append('')
        if features['audio_spectrum']:
            lines.append('#define MAX_AUDIO_SPECTRUM 512')
            lines.append('uniform float u_audioSpectrum[MAX_AUDIO_SPECTRUM];')
            lines.append('uniform int u_spectrumCount;')
            lines.append('')

        param_uniforms = generate_uniforms(params)
        if param_uniforms:
            lines.append(param_uniforms)
            lines.append('')

    # KSL standard library functions (only those used by this shader)
    if stdlib_funcs:
        lines.append('// KSL standard library')
        for fname in stdlib_funcs:
            lines.append(KSL_STDLIB[fname])
            lines.append('')

    # Helper functions
    for helper in helpers:
        transformed = transform_helper(helper, input_var, params)
        lines.append(transformed)
        lines.append('')

    # Shade body as a wrapper function (preserves early returns)
    transformed_body = transform_body(body, input_var, params)

    lines.append('vec4 ksl_shade() {')
    for line in transformed_body.split('\n'):
        lines.append('    ' + line.rstrip() if line.strip() else '')
    lines.append('}')
    lines.append('')

    # Main function calls wrapper
    lines.append('void main() {')
    lines.append('    fragColor = ksl_shade();')
    lines.append('}')

    return '\n'.join(lines)


def generate_uber_glsl(shader_sources):
    """Generate a single uber-shader containing all KSL shade functions.

    Args:
        shader_sources: list of (name, source_text) tuples

    Returns:
        Complete GLSL fragment shader source with all shade functions
        and a dispatch main() that selects by u_shaderID uniform.
    """
    # Collect data from all shaders
    shader_data = []
    all_features = {
        'lights': False, 'textures': False, 'audio_samples': False,
        'audio_spectrum': False, 'time': False, 'depth': False,
    }
    all_stdlib = set()
    all_uniforms = {}  # name -> (glsl_type, array_size or None)
    all_texture_indices = set()

    for name, source in shader_sources:
        params = extract_params(source)
        body, input_var = extract_shade_body(source)
        helpers = extract_helper_functions(source)
        features = detect_features(source, body)
        stdlib_funcs = detect_stdlib_usage(source)

        # Merge features
        for k, v in features.items():
            if v:
                all_features[k] = True

        # Merge stdlib
        all_stdlib.update(stdlib_funcs)

        # Merge uniforms (all shared names have same types - verified)
        for typ, pname, arr_size in params:
            glsl_type = typ.replace('ksl::', '')
            all_uniforms[f'u_{pname}'] = (glsl_type, arr_size)

        # Collect texture indices
        for i in range(8):
            if f'in.ctx->textures[{i}]' in source or f'u_texture{i}' in source:
                all_texture_indices.add(i)

        # Transform body and helpers
        transformed_body = transform_body(body, input_var, params)
        transformed_helpers = []
        for helper in helpers:
            transformed_helpers.append(transform_helper(helper, input_var, params))

        shader_data.append({
            'name': name,
            'body': transformed_body,
            'helpers': transformed_helpers,
        })

    # Resolve stdlib dependency order
    ordered_stdlib = []
    remaining = set(all_stdlib)
    while remaining:
        for fname in list(remaining):
            deps = set(KSL_STDLIB_DEPS.get(fname, []))
            if deps.issubset(set(ordered_stdlib)):
                ordered_stdlib.append(fname)
                remaining.remove(fname)

    # --- Generate GLSL ---
    lines = []
    lines.append('// Auto-generated KSL uber-shader - DO NOT EDIT')
    lines.append('#version 330 core')
    lines.append('')

    # Varyings
    lines.append('in vec3 v_position;')
    lines.append('in vec3 v_normal;')
    lines.append('in vec2 v_uv;')
    lines.append('in vec3 v_viewDir;')
    if all_features['depth']:
        lines.append('in float v_depth;')
    lines.append('')
    lines.append('out vec4 fragColor;')
    lines.append('')

    # Dispatch uniform
    lines.append('uniform int u_shaderID;')
    lines.append('')

    # Time uniforms
    if all_features['time']:
        lines.append('uniform float u_time;')
        lines.append('uniform float u_dt;')
        lines.append('uniform int u_frameCount;')
        lines.append('')

    # Light uniforms
    if all_features['lights']:
        lines.append('#define MAX_LIGHTS 16')
        lines.append('struct Light {')
        lines.append('    vec3 position;')
        lines.append('    vec3 color;')
        lines.append('    float intensity;')
        lines.append('    float falloff;')
        lines.append('    float curve;')
        lines.append('};')
        lines.append('uniform Light u_lights[MAX_LIGHTS];')
        lines.append('uniform int u_lightCount;')
        lines.append('')

    # Texture uniforms
    if all_features['textures']:
        for i in sorted(all_texture_indices):
            lines.append(f'uniform sampler2D u_texture{i};')
        if not all_texture_indices:
            lines.append('uniform sampler2D u_texture0;')
        lines.append('')

    # Audio uniforms (reduced size for uber to fit GL 3.3 uniform limits)
    if all_features['audio_samples']:
        lines.append('#define MAX_AUDIO_SAMPLES 256')
        lines.append('uniform float u_audioSamples[MAX_AUDIO_SAMPLES];')
        lines.append('uniform int u_sampleCount;')
        lines.append('')
    if all_features['audio_spectrum']:
        lines.append('#define MAX_AUDIO_SPECTRUM 128')
        lines.append('uniform float u_audioSpectrum[MAX_AUDIO_SPECTRUM];')
        lines.append('uniform int u_spectrumCount;')
        lines.append('')

    # All shader-specific uniforms (superset, deduplicated)
    if all_uniforms:
        lines.append('// Shader parameters (superset of all KSL shaders)')
        for uname in sorted(all_uniforms.keys()):
            glsl_type, arr_size = all_uniforms[uname]
            if arr_size:
                lines.append(f'uniform {glsl_type} {uname}[{arr_size}];')
            else:
                lines.append(f'uniform {glsl_type} {uname};')
        lines.append('')

    # Camera uniform (used by light shaders)
    lines.append('uniform vec3 u_cameraPos;')
    lines.append('')

    # KSL stdlib functions
    if ordered_stdlib:
        lines.append('// KSL standard library')
        for fname in ordered_stdlib:
            lines.append(KSL_STDLIB[fname])
            lines.append('')

    # Emit helper functions (deduplicated by text)
    emitted_helpers = set()
    for sd in shader_data:
        for helper in sd['helpers']:
            h_stripped = helper.strip()
            if h_stripped not in emitted_helpers:
                emitted_helpers.add(h_stripped)
                lines.append(helper)
                lines.append('')

    # Emit per-shader shade functions
    for i, sd in enumerate(shader_data):
        lines.append(f'// Shader {i}: {sd["name"]}')
        lines.append(f'vec4 ksl_shade_{sd["name"]}() {{')
        for line in sd['body'].split('\n'):
            lines.append('    ' + line.rstrip() if line.strip() else '')
        lines.append('}')
        lines.append('')

    # Dispatch main()
    lines.append('void main() {')
    for i, sd in enumerate(shader_data):
        cond = 'if' if i == 0 else '} else if'
        lines.append(f'    {cond} (u_shaderID == {i}) {{')
        lines.append(f'        fragColor = ksl_shade_{sd["name"]}();')
    lines.append('    } else {')
    lines.append('        fragColor = vec4(1.0, 0.0, 0.78, 1.0); // pink error')
    lines.append('    }')
    lines.append('}')

    # Build name -> ID mapping
    shader_ids = {sd['name']: i for i, sd in enumerate(shader_data)}

    return '\n'.join(lines), shader_ids


def main():
    parser = argparse.ArgumentParser(description='KSL -> GLSL codegen')
    parser.add_argument('input', nargs='+', help='.ksl.hpp input file(s)')
    parser.add_argument('-o', '--output', help='Output .glsl file (default: stdout)')
    parser.add_argument('-n', '--name', help='Shader name override')
    parser.add_argument('--uber', action='store_true',
                        help='Generate uber-shader from all input files')
    parser.add_argument('--uber-ids', help='Output shader ID mapping file (.txt)')
    parser.add_argument('--vulkan', action='store_true',
                        help='Generate Vulkan-compatible GLSL (#version 450, '
                             'explicit layout qualifiers, descriptor set bindings)')
    args = parser.parse_args()

    if args.uber:
        if args.vulkan:
            print('Error: --uber and --vulkan cannot be combined '
                  '(Vulkan does not use uber-shaders)', file=sys.stderr)
            sys.exit(1)
        # Uber-shader mode: combine all inputs into one shader
        shader_sources = []
        for inp in sorted(args.input):
            p = Path(inp)
            source = p.read_text()
            # Derive shader name from filename: foo.ksl.hpp -> foo
            stem = p.stem  # foo.ksl
            if stem.endswith('.ksl'):
                stem = stem[:-4]
            shader_sources.append((stem, source))

        glsl, shader_ids = generate_uber_glsl(shader_sources)

        if args.output:
            Path(args.output).write_text(glsl)
            print(f'Generated uber-shader {args.output} ({len(shader_ids)} shaders)')
        else:
            print(glsl)

        if args.uber_ids:
            with open(args.uber_ids, 'w') as f:
                for name, sid in sorted(shader_ids.items(), key=lambda x: x[1]):
                    f.write(f'{sid} {name}\n')
            print(f'Wrote shader ID map to {args.uber_ids}')
    else:
        # Single-shader mode
        source = Path(args.input[0]).read_text()
        glsl = generate_glsl(source, args.name, vulkan=args.vulkan)

        if args.output:
            Path(args.output).write_text(glsl)
            suffix = ' (Vulkan)' if args.vulkan else ''
            print(f'Generated {args.output}{suffix}')
        else:
            print(glsl)


if __name__ == '__main__':
    main()
