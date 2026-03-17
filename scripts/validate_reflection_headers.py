#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""
Validate reflection header compatibility.

Scans engine/koilo/ for .hpp files and reports any that would be
excluded from code generation due to template<> or virtual keywords.

Usage:
    python3 scripts/validate_reflection_headers.py           # report all
    python3 scripts/validate_reflection_headers.py --check   # exit 1 if violations found
    python3 scripts/validate_reflection_headers.py --diff     # only check git-changed files
"""

import argparse
import os
import re
import subprocess
import sys

TEMPLATE_RE = re.compile(r'\btemplate\s*<')
VIRTUAL_RE  = re.compile(r'\bvirtual\b')

# Files intentionally excluded (abstract bases, architectural necessity)
KNOWN_EXCLUSIONS = {
    # Abstract bases / interfaces (virtual by design)
    "mathematics.hpp",
    "entitymanager.hpp",
    "resourcehandle.hpp",
    "resourcemanager.hpp",
    "imaterial.hpp",
    "ishader.hpp",
    "idisplaybackend.hpp",
    "shape.hpp",
    "system.hpp",
    "collider.hpp",
    "timeline.hpp",
    "selectornode.hpp",
    "sequencenode.hpp",
    "actionnode.hpp",
    "conditionnode.hpp",
    "materialt.hpp",
    "istatictrianglegroup.hpp",
    "itrianglegroup.hpp",
    "color.hpp",
    "ipixelgroup.hpp",
    "camerabase.hpp",
    "iscriptfilereader.hpp",
    "effect.hpp",
    "project.hpp",
    "behaviortreenode.hpp",
    "behaviortreeaction.hpp",
    # Internal infrastructure (template/virtual for engine internals)
    "reflect_helpers.hpp",
    "reflect_make.hpp",
    "reflect_macros.hpp",
    "type_registry.hpp",
    "registry.hpp",
    "koiloscript_ast.hpp",
    "koiloscript_engine.hpp",
    "koiloscript_parser.hpp",
    "component.hpp",
    "quadtree.hpp",
    "koilo_shm.hpp",
    "scenedata.hpp",
    # Render / KSL internals (virtual or template by design)
    "irenderbackend.hpp",
    "opengl_render_backend.hpp",
    "ksl_elf_loader.hpp",
    "ksl_shader.hpp",
    "module_api.hpp",
    "animationtrack.hpp",
    # UI internals
    "undo_stack.hpp",
    # Scripting internals (inner struct with nullptr member init breaks regex parser)
    "bytecode_vm.hpp",
}


def scan_file(filepath):
    """Return list of (line_number, reason) violations."""
    violations = []
    try:
        with open(filepath, 'r', encoding='utf-8', errors='replace') as f:
            for i, line in enumerate(f, 1):
                if TEMPLATE_RE.search(line):
                    violations.append((i, "template<", line.rstrip()))
                if VIRTUAL_RE.search(line):
                    violations.append((i, "virtual", line.rstrip()))
    except OSError:
        pass
    return violations


def gather_headers(root):
    """Yield all .hpp files under root."""
    for dirpath, _, filenames in os.walk(root):
        for fn in sorted(filenames):
            if fn.endswith('.hpp'):
                yield os.path.join(dirpath, fn)


def git_changed_files(root):
    """Return set of changed .hpp files (staged + unstaged)."""
    try:
        out = subprocess.check_output(
            ['git', 'diff', '--name-only', '--diff-filter=ACMR', 'HEAD'],
            cwd=root, text=True, stderr=subprocess.DEVNULL
        )
        staged = subprocess.check_output(
            ['git', 'diff', '--cached', '--name-only', '--diff-filter=ACMR'],
            cwd=root, text=True, stderr=subprocess.DEVNULL
        )
        files = set(out.strip().split('\n') + staged.strip().split('\n'))
        return {f for f in files if f.endswith('.hpp')}
    except (subprocess.CalledProcessError, FileNotFoundError):
        return set()


def main():
    parser = argparse.ArgumentParser(description="Validate reflection header compatibility")
    parser.add_argument('--check', action='store_true',
                        help="Exit with code 1 if new violations found")
    parser.add_argument('--diff', action='store_true',
                        help="Only check git-changed files")
    parser.add_argument('--root', default='engine/koilo',
                        help="Root directory to scan")
    args = parser.parse_args()

    repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    scan_root = os.path.join(repo_root, args.root)

    if not os.path.isdir(scan_root):
        print(f"Error: {scan_root} not found", file=sys.stderr)
        sys.exit(1)

    if args.diff:
        changed = git_changed_files(repo_root)
        headers = [os.path.join(repo_root, f) for f in changed
                    if f.startswith(args.root)]
    else:
        headers = list(gather_headers(scan_root))

    excluded_known = []
    excluded_new = []
    clean = 0

    for filepath in headers:
        basename = os.path.basename(filepath)
        violations = scan_file(filepath)
        relpath = os.path.relpath(filepath, repo_root)

        if not violations:
            clean += 1
            continue

        if basename in KNOWN_EXCLUSIONS:
            excluded_known.append((relpath, violations))
        else:
            excluded_new.append((relpath, violations))

    # Report
    print(f"Scanned {len(headers)} headers\n")

    if excluded_new:
        print(f"⚠  {len(excluded_new)} file(s) with reflection-breaking constructs:")
        print("   (These will be silently excluded from KoiloScript reflection)\n")
        for relpath, violations in sorted(excluded_new):
            print(f"  {relpath}")
            for line_no, reason, text in violations[:3]:
                print(f"    L{line_no}: [{reason}] {text.strip()}")
            if len(violations) > 3:
                print(f"    ... and {len(violations) - 3} more")
        print()

    if excluded_known:
        print(f"ℹ  {len(excluded_known)} known exclusion(s) (intentional):")
        for relpath, _ in sorted(excluded_known):
            print(f"  {relpath}")
        print()

    print(f"✓  {clean} header(s) are reflection-compatible")

    if args.check and excluded_new:
        print(f"\n✗  Found {len(excluded_new)} unexpected exclusion(s). "
              "Fix or add to KNOWN_EXCLUSIONS in this script.")
        sys.exit(1)


if __name__ == '__main__':
    main()
