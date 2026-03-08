#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Generate a reflection_entry_gen.cpp that calls Describe() for every class
with KL_BEGIN_DESCRIBE. This script attempts to produce qualified class
names (including namespaces) by locating the class/struct declaration and
scanning for enclosing namespace declarations.

Usage: generate_reflection_entry.py --root engine/koilo --output build/generated/reflection_entry_gen.cpp
"""
from pathlib import Path
import re
import sys

# Add scripts directory to path for consoleoutput module
SCRIPT_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(SCRIPT_DIR))

from consoleoutput import print_progress, print_status, print_section, print_warning, print_error, print_success, Colors

repo = Path(__file__).resolve().parents[1]

def find_candidates(root: Path):
    pattern = re.compile(r'KL_(?:BEGIN|DECLARE)_DESCRIBE\s*\(\s*([A-Za-z_][A-Za-z0-9_:]*)\s*\)')
    candidates = []
    for p in root.rglob('*.hpp'):
        try:
            txt = p.read_text(encoding='utf-8')
        except Exception:
            continue

        # Filter out files that are purely template/virtual without explicit Describe
        # BUT: if the class has KL_BEGIN_DESCRIBE/KL_DECLARE_DESCRIBE AND is also in koiloall.hpp, include it
        has_describe = 'KL_BEGIN_DESCRIBE' in txt or 'KL_DECLARE_DESCRIBE' in txt
        if not has_describe:
            if re.search(r'\btemplate\s*<', txt):
                continue
            if re.search(r'\bvirtual\b', txt):
                continue

        for m in pattern.finditer(txt):
            raw = m.group(1)
            if raw.upper() in ('CLASS', 'INTERFACE', 'STRUCT', 'ENUM'):
                continue
            candidates.append((p, raw, txt))
    return candidates


def qualify_name(raw: str, txt: str):
    # If raw already contains :: assume it's fully qualified
    if '::' in raw:
        return raw

    # Try to find a top-level declaration for the candidate and any enclosing namespaces
    candidate = raw
    # Search for a class/struct declaration for candidate
    decl_re = re.compile(r'^[ \t]*(class|struct)\s+' + re.escape(candidate) + r'\b', re.M)
    m = decl_re.search(txt)
    if not m:
        # fallback to unqualified
        return candidate

    decl_pos = m.start()

    # find enclosing class/struct declarations (for nested types like Shape::Bounds)
    enclosing_re = re.compile(r'\b(class|struct)\s+([A-Za-z_][A-Za-z0-9_]*)\b')
    enclosing_candidates = []
    for em in enclosing_re.finditer(txt[:decl_pos]):
        # Skip matches inside comments (line starts with *, //, or is in a doxygen block)
        line_start = txt.rfind('\n', 0, em.start()) + 1
        line_prefix = txt[line_start:em.start()].lstrip()
        if line_prefix.startswith('*') or line_prefix.startswith('//') or line_prefix.startswith('/*'):
            continue
        # Skip forward declarations (struct Foo;  no body brace)
        semi_idx = txt.find(';', em.end())
        brace_idx = txt.find('{', em.end())
        if semi_idx != -1 and (brace_idx == -1 or semi_idx < brace_idx):
            continue
        # Skip matches that are part of an 'enum class/struct' declaration.
        prefix = txt[:em.start()].rstrip()
        m_prev = re.search(r'([A-Za-z_][A-Za-z0-9_]*)$', prefix)
        if m_prev and m_prev.group(1) == 'enum':
            continue
        enclosing_candidates.append(em)
    # Check from innermost to outermost
    for last_enclosing in reversed(enclosing_candidates):
        # ensure the enclosing declaration actually opens a '{' before decl_pos
        part = txt[last_enclosing.end():decl_pos]
        opens = part.count('{')
        closes = part.count('}')
        if opens > closes:
            parent = last_enclosing.group(2)
            # Heuristic: detect whether the nested declaration lives in a private
            # section of the parent by looking for the last 'public:'/'private:'
            # labels between the parent start and the nested declaration.
            section_span = txt[last_enclosing.end():decl_pos]
            last_public = section_span.rfind('public:')
            last_private = section_span.rfind('private:')
            if last_private > last_public:
                # nested type is private; skip emitting a call (can't access)
                return None
            nested_name = f"{parent}::{candidate}"
            # Also find enclosing namespaces for the parent class
            ns_re2 = re.compile(r'(?<!@)\bnamespace\s+([A-Za-z_][A-Za-z0-9_:]*)')
            namespaces = []
            parent_pos = last_enclosing.start()
            for nm in ns_re2.finditer(txt[:parent_pos]):
                brace_idx = txt.find('{', nm.end())
                if brace_idx != -1 and brace_idx < parent_pos:
                    part2 = txt[nm.end():parent_pos]
                    if part2.count('{') > part2.count('}'):
                        for seg in nm.group(1).split('::'):
                            if seg:
                                namespaces.append(seg)
            if namespaces:
                namespaces = [ns for ns in namespaces if ns not in ('std', 'detail')]
                return '::'.join(namespaces + [nested_name])
            return nested_name

    # fallback: try to detect enclosing namespaces
    ns_re = re.compile(r'(?<!@)\bnamespace\s+([A-Za-z_][A-Za-z0-9_:]*)')
    namespaces = []
    for nm in ns_re.finditer(txt[:decl_pos]):
        brace_idx = txt.find('{', nm.end())
        if brace_idx != -1 and brace_idx < decl_pos:
            part = txt[nm.end():decl_pos]
            if part.count('{') > part.count('}'):
                for seg in nm.group(1).split('::'):
                    if seg:
                        namespaces.append(seg)

    if namespaces:
        # Filter out standard library namespaces that may appear in the file
        namespaces = [ns for ns in namespaces if ns not in ('std', 'detail')]
        # Deduplicate: forward decl patterns like
        # `namespace koilo { namespace scripting { struct X; } }` followed by
        # `namespace koilo { namespace scripting {` cause doubled segments.
        # Only keep unique segments in order of first appearance.
        deduped = list(dict.fromkeys(namespaces))
        qualified = '::'.join(deduped + [candidate])
        return qualified

    return candidate


def build_class_list(root: Path):
    candidates = find_candidates(root)
    class_names = set()
    for path, raw, txt in candidates:
        q = qualify_name(raw, txt)
        if q:
            class_names.add(q)
    return sorted(class_names)


def is_platform_class(cls_name, candidates):
    """Check if a class is platform-specific (display backends, application, renderer,
    or types that directly depend on DisplayManager)."""
    # Classes that depend on platform/display types
    PLATFORM_CLASSES = {'LoadedScene', 'SceneLoader'}

    for path, raw, txt in candidates:
        q = qualify_name(raw, txt)
        if q == cls_name or (q and f'koilo::{q}' == cls_name):
            rel = str(path)
            # Platform-specific: display backends, application, rendering engine
            if '/systems/display/' in rel:
                return True
            if '/koiloapplication.' in rel:
                return True
            if '/systems/render/engine/' in rel:
                return True
            # Classes with direct DisplayManager dependency
            if raw in PLATFORM_CLASSES:
                return True
            return False
    return False


def classify_classes(classes, candidates):
    """Split classes into core and platform lists."""
    core = []
    platform = []
    for c in classes:
        if is_platform_class(c, candidates):
            platform.append(c)
        else:
            core.append(c)
    return core, platform


def _validate_class(c):
    """Validate and fix class name. Returns (valid_name, skip_reason) or (None, reason)."""
    if '::' in c and not c.startswith('koilo::'):
        if 'defines::' in c or 'contains::' in c:
            return None, "malformed namespace path"
        # ksl:: is a standalone namespace, not nested inside koilo::
        if c.startswith('ksl::'):
            return None, "standalone ksl namespace"
        return f'koilo::{c}', None
    return c, None


def _write_registration_file(out, classes, include_header, function_name, label):
    """Write a single registration .cpp file."""
    out.parent.mkdir(parents=True, exist_ok=True)
    skipped = []
    valid_classes = []

    for c in classes:
        fixed, reason = _validate_class(c)
        if fixed is None:
            skipped.append((c, reason))
        else:
            valid_classes.append(fixed)

    with out.open('w', encoding='utf-8') as fp:
        fp.write(f'// Auto-generated {label} registry (generated by scripts/generatereflectionentry.py)\n')
        fp.write(f'#include <{include_header}>\n')
        fp.write('#include <koilo/registry/ensure_registration.hpp>\n\n')
        fp.write('namespace {\n')
        fp.write(f'struct _AutoDescribe{label.replace(" ", "")} {{\n')
        fp.write(f'    _AutoDescribe{label.replace(" ", "")}() {{\n')
        for s, reason in skipped:
            fp.write(f'        // Skipped: {s} ({reason})\n')
        for c in valid_classes:
            fp.write(f'        (void){c}::Describe();\n')
        fp.write('    }\n')
        fp.write('};\n')
        fp.write(f'static _AutoDescribe{label.replace(" ", "")} _auto_describe_{label.replace(" ", "_").lower()}_instance;\n')
        fp.write('} // anonymous namespace\n\n')
        fp.write(f'// Public function to force linkage of this TU\n')
        fp.write(f'namespace koilo {{ namespace registry {{\n')
        fp.write(f'void {function_name}() {{\n')
        fp.write(f'    (void)&_auto_describe_{label.replace(" ", "_").lower()}_instance;\n')
        fp.write('}\n')
        fp.write('} } // namespace koilo::registry\n')
    print_success(f'   Wrote {out} with {len(valid_classes)} Describe() calls ({len(skipped)} skipped) [{label}]')
    return len(valid_classes), len(skipped)


def write_output(out: Path, classes, candidates=None):
    """Write split core + platform registration files."""
    if candidates is not None:
        core_classes, platform_classes = classify_classes(classes, candidates)
    else:
        core_classes, platform_classes = classes, []

    # Write core file (always linked - safe for headless)
    core_out = out.parent / 'reflection_core_gen.cpp'
    _write_registration_file(core_out, core_classes, 'koilo/koiloall.hpp',
                             'EnsureCoreReflectionRegistered', 'Core')

    # Write platform file (only linked by hosts with display backends)
    if platform_classes:
        platform_out = out.parent / 'reflection_platform_gen.cpp'
        _write_registration_file(platform_out, platform_classes, 'koilo/koiloall.hpp',
                                 'EnsurePlatformReflectionRegistered', 'Platform')

    # Write combined file for backward compatibility
    out.parent.mkdir(parents=True, exist_ok=True)
    skipped = []
    valid_classes = []
    for c in classes:
        fixed, reason = _validate_class(c)
        if fixed is None:
            skipped.append((c, reason))
        else:
            valid_classes.append(fixed)

    with out.open('w', encoding='utf-8') as fp:
        fp.write('// Auto-generated registry translation unit (generated by scripts/generatereflectionentry.py)\n')
        fp.write('// NOTE: Prefer reflection_core_gen.cpp for headless builds.\n')
        fp.write('#include <koilo/koiloall.hpp>\n')
        fp.write('#include <koilo/registry/ensure_registration.hpp>\n\n')
        fp.write('namespace {\n')
        fp.write('struct _AutoDescribe {\n')
        fp.write('    _AutoDescribe() {\n')
        for s, reason in skipped:
            fp.write(f'        // Skipped: {s} ({reason})\n')
        for c in valid_classes:
            fp.write(f'        (void){c}::Describe();\n')
        fp.write('    }\n')
        fp.write('};\n')
        fp.write('static _AutoDescribe _auto_describe_instance;\n')
        fp.write('} // anonymous namespace\n\n')
        fp.write('// Public function to force linkage of this TU\n')
        fp.write('namespace koilo { namespace registry {\n')
        fp.write('void EnsureReflectionRegistered() {\n')
        fp.write('    // Force reference to static initializer to ensure it runs\n')
        fp.write('    (void)&_auto_describe_instance;\n')
        fp.write('}\n')
        fp.write('} } // namespace koilo::registry\n')
    print_success(f'   Wrote {out} with {len(valid_classes)} Describe() calls ({len(skipped)} skipped)')


def main():
    import argparse
    ap = argparse.ArgumentParser()
    ap.add_argument('--root', required=True)
    ap.add_argument('--output', required=True)
    ns = ap.parse_args()
    root = Path(ns.root)
    out = Path(ns.output)

    print_section("Generating reflection entry files...")
    print_status(f"   Scanning: {root}", Colors.GREEN)

    candidates = find_candidates(root)
    classes = build_class_list(root)
    if not classes:
        print_warning('No classes found (no KL_BEGIN_DESCRIBE)')
        sys.exit(0)

    write_output(out, classes, candidates)


if __name__ == '__main__':
    main()
