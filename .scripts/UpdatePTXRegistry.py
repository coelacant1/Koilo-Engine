# UpdatePTXRegistry.py - auto-generate PTX reflection blocks from C++ headers
#
# Process
# - Walk all .hpp under a root directory (default: lib/ptx)
# - Parse classes (normal + templates) and their public API:
#     * public non-static fields  -> PTX_FIELD
#     * public methods            -> PTX_METHOD_AUTO / PTX_SMETHOD_AUTO
#     * constructors              -> PTX_CTOR / PTX_CTOR0
# - Insert blocks after each class definition (or reorder/fix if present)
# - Creates .bak backups when writing
#
# Parsing
# - Preferred: libclang, pass extra compile args via --clang-arg.
# - Fallback: regex parser that understands 'template<...> class/struct ... { ... };'
#
# Usage examples
#   python UpdatePTXRegistry.py --root ./lib/ptx --write --verbose
#   python UpdatePTXRegistry.py --root ./lib/ptx --write --only RGBColor Vector3D
#   python UpdatePTXRegistry.py --root ./lib/ptx --write --clang-arg -I./lib --clang-arg -std=c++17
#

import argparse
import re
import sys
import os
from dataclasses import dataclass, field
from pathlib import Path
from typing import List, Optional, Tuple, Dict

# ------------------------ Type ranges & doc helpers ------------------------
TYPE_RANGES: Dict[str, Tuple[str, str]] = {
    "bool": ("0", "1"),
    "char": ("-128", "127"),
    "signed char": ("-128", "127"),
    "unsigned char": ("0", "255"),
    "int8_t": ("-128", "127"),
    "uint8_t": ("0", "255"),
    "short": ("-32768", "32767"),
    "unsigned short": ("0", "65535"),
    "int16_t": ("-32768", "32767"),
    "uint16_t": ("0", "65535"),
    "int": ("-2147483648", "2147483647"),
    "unsigned int": ("0", "4294967295"),
    "int32_t": ("-2147483648", "2147483647"),
    "uint32_t": ("0", "4294967295"),
    "long": ("-9223372036854775808", "9223372036854775807"),
    "unsigned long": ("0", "18446744073709551615"),
    "int64_t": ("-9223372036854775808", "9223372036854775807"),
    "uint64_t": ("0", "18446744073709551615"),
    "float": ("__FLT_MIN__", "__FLT_MAX__"),
    "double": ("__DBL_MIN__", "__DBL_MAX__"),
}

INCLUDE_RE = re.compile(r'^\s*#\s*include\s*[<"]([^">]+reflect_macros\.hpp)[">]', re.MULTILINE)

def ensure_reflect_include(src: str, header_path: Path, root_path: Path) -> tuple[str, bool, str]:
    """
    Ensure the file includes the PTX macros header with a relative path:
      #include "<rel-to-file>/registry/reflect_macros.hpp"
    Returns (new_src, changed, relpath_used).
    """
    if INCLUDE_RE.search(src):
        return src, False, ""

    macros_abspath = (root_path / "registry" / "reflect_macros.hpp").resolve()
    rel = os.path.relpath(macros_abspath, start=header_path.parent.resolve()).replace("\\", "/")
    include_line = f'#include "{rel}"\n'

    lines = src.splitlines(keepends=True)
    idx = 0

    for i, line in enumerate(lines[:80]):
        if re.match(r'^\s*#\s*pragma\s+once\b', line):
            idx = i + 2
            while idx < len(lines) and re.match(r'^\s*#\s*include\b', lines[idx]):
                idx += 1
            break
    else:
        guard_ifndef = guard_define = None
        for i, line in enumerate(lines[:120]):
            if guard_ifndef is None and re.match(r'^\s*#\s*ifndef\b', line):
                guard_ifndef = i
            elif guard_ifndef is not None and re.match(r'^\s*#\s*define\b', line):
                guard_define = i
                break
        if guard_define is not None:
            idx = guard_define + 1
            while idx < len(lines) and re.match(r'^\s*#\s*include\b', lines[idx]):
                idx += 1
        else:
            idx = 0
            while idx < len(lines) and (lines[idx].strip() == "" or lines[idx].lstrip().startswith("//") or lines[idx].lstrip().startswith("/*")):
                idx += 1

    if idx > 0 and not lines[idx-1].endswith("\n"):
        include_line = "\n" + include_line

    lines.insert(idx, include_line)
    return "".join(lines), True, rel

def guess_range(ty: str) -> Tuple[str, str]:
    t = ty.strip()
    t = re.sub(r"\s+", " ", t)
    t_norm = t.replace("const ", "").replace("&", "").replace("*", "").strip()
    t_norm = t_norm.replace("unsigned char", "unsigned char").replace("unsigned int", "unsigned int")
    return TYPE_RANGES.get(t_norm, ("0", "0"))

def nice_doc_from_name(name: str) -> str:
    s = name
    s = re.sub(r"(_)+", " ", s)
    s = re.sub(r"(?<=[a-z0-9])([A-Z])", r" \1", s)
    s = s.strip().capitalize()
    return s or name

# ------------------------ Data models ------------------------
@dataclass
class FieldInfo:
    name: str
    type: str

@dataclass
class MethodInfo:
    name: str
    is_static: bool

@dataclass
class CtorInfo:
    args: List[str] = field(default_factory=list)

@dataclass
class ClassInfo:
    name: str
    name_token: str
    start: int
    close_brace: int
    after_semicolon: int
    is_template: bool
    fields: List[FieldInfo] = field(default_factory=list)
    methods: List[MethodInfo] = field(default_factory=list)
    ctors: List[CtorInfo] = field(default_factory=list)

# ------------------------ libclang front-end (preferred when available) ------------------------
def try_clang_parse(paths: List[Path], clang_args: List[str]) -> Dict[Path, List[ClassInfo]]:
    try:
        from clang import cindex
    except Exception:
        return {}

    index = cindex.Index.create()
    results: Dict[Path, List[ClassInfo]] = {}

    path_to_text = {p: p.read_text(encoding="utf-8", errors="ignore") for p in paths}

    def _lc_offset(text: str, loc):
        line = max(1, getattr(loc, "line", 1))
        col  = max(1, getattr(loc, "column", 1))
        i = 0
        cur = 1
        while cur < line and i < len(text):
            j = text.find('\n', i)
            if j == -1: return len(text)
            i = j + 1
            cur += 1
        return min(len(text), i + (col - 1))
    
    pcount = 0

    for p in paths:
        tu = index.parse(str(p), args=clang_args)
        if not tu:
            continue

        src = path_to_text[p]
        out_list: List[ClassInfo] = []

        def visit(cursor):
            for c in cursor.get_children():
                if c.kind not in (
                    cindex.CursorKind.CLASS_DECL,
                    cindex.CursorKind.STRUCT_DECL,
                    cindex.CursorKind.CLASS_TEMPLATE,
                ):
                    visit(c)
                    continue
                if not c.is_definition():
                    visit(c)
                    continue

                start_file = getattr(c.extent.start, "file", None)
                if start_file is None or Path(start_file.name) != p:
                    visit(c)
                    continue

                start = getattr(c.extent.start, "offset", None)
                end   = getattr(c.extent.end, "offset", None)
                if start is None: start = _lc_offset(src, c.extent.start)
                if end   is None: end   = _lc_offset(src, c.extent.end)

                open_to_close_src = src[:end]
                close_brace = open_to_close_src.rfind('}', start, end)
                after_semicolon = end

                fields: List[FieldInfo] = []
                methods: List[MethodInfo] = []
                ctors: List[CtorInfo] = []

                for ch in c.get_children():
                    if getattr(ch.location, "file", None) and Path(ch.location.file.name) != p:
                        continue
                    if ch.access_specifier != cindex.AccessSpecifier.PUBLIC:
                        continue
                    if ch.kind == cindex.CursorKind.FIELD_DECL:
                        fields.append(FieldInfo(name=ch.spelling, type=ch.type.spelling))
                    elif ch.kind == cindex.CursorKind.CXX_METHOD:
                        nm = ch.spelling
                        if nm.startswith("operator") or nm.startswith("~"):
                            continue
                        methods.append(MethodInfo(name=nm, is_static=ch.is_static_method()))
                    elif ch.kind == cindex.CursorKind.CONSTRUCTOR:
                        arg_types = [arg.type.spelling for arg in ch.get_arguments()]
                        ctors.append(CtorInfo(args=arg_types))

                is_templ = (c.kind == cindex.CursorKind.CLASS_TEMPLATE)
                name_text = c.spelling
                name_token = c.spelling

                out_list.append(ClassInfo(
                    name=name_text,
                    name_token=name_token,
                    start=start,
                    close_brace=close_brace if close_brace != -1 else end,
                    after_semicolon=end,
                    is_template=is_templ,
                    fields=fields,
                    methods=methods,
                    ctors=ctors
                ))

                visit(c)

        visit(tu.cursor)
        if out_list:
            results[p] = out_list

            print(f"{pcount} of {len(paths)} - Parsing clang of {p}")
        else:
            print(f"{pcount} of {len(paths)} - Skipping {p}")
        
        pcount += 1

    return results


# ------------------------ regex fallback parser ------------------------
CLASS_RE = re.compile(
    r"""
    (?P<templ>template\s*<[^>]+>\s*)?
    (?P<kw>class|struct)\s+
    (?P<name>[A-Za-z_]\w*(?:\s*<[^{};>]+>)?)
    (?:\s*:[^{]+)?
    \s*\{
    """,
    re.VERBOSE | re.DOTALL,
)

ACCESS_RE = re.compile(r"^\s*(public|protected|private)\s*:\s*$", re.MULTILINE)

def find_matching_brace(src: str, open_pos: int) -> int:
    depth = 0
    i = open_pos
    n = len(src)
    in_s = in_c = False
    while i < n:
        ch = src[i]
        if ch == '"' and not in_c:
            in_s = not in_s
        elif ch == "'" and not in_s:
            in_c = not in_c
        elif not in_s and not in_c:
            if src.startswith("//", i):
                j = src.find("\n", i)
                if j == -1: return n - 1
                i = j; continue
            if src.startswith("/*", i):
                j = src.find("*/", i + 2)
                if j == -1: return n - 1
                i = j + 2; continue
            if ch == '{':
                depth += 1
            elif ch == '}':
                depth -= 1
                if depth == 0:
                    return i
        i += 1
    return n - 1

def parse_public_api_from_body(body: str, cls_name: str) -> Tuple[List[FieldInfo], List[MethodInfo], List[CtorInfo]]:
    fields: List[FieldInfo] = []
    methods: List[MethodInfo] = []
    ctors: List[CtorInfo] = []

    sections = []
    last = 0
    for m in ACCESS_RE.finditer(body):
        sections.append((last, m.start(), "implicit"))
        sections.append((m.end(), None, m.group(1)))
        last = m.end()
    sections.append((last, None, "implicit"))

    public_spans: List[Tuple[int, int]] = []
    prev_end = 0
    for i in range(0, len(sections), 2):
        start, end, kind = sections[i]
        if end is None:
            end = len(body)
        if i + 1 < len(sections):
            nxt = sections[i + 1]
            access_word = nxt[2]
            next_m = ACCESS_RE.search(body, nxt[0])
            nxt_end = next_m.start() if next_m else len(body)
            if access_word == "public":
                public_spans.append((nxt[0], nxt_end))

    if not public_spans and "public:" not in body:
        public_spans = [(0, len(body))]

    decl_re = re.compile(r";")
    for (s, e) in public_spans:
        segment = body[s:e]

        idx = 0
        while True:
            m = decl_re.search(segment, idx)
            if not m:
                break
            stmt = segment[idx:m.start()].strip()
            idx = m.end()
            if not stmt:
                continue
            if re.match(r"^\s*(public|private|protected)\s*:$", stmt):
                continue
            if re.match(r"^\s*(class|struct|enum)\b", stmt):
                continue
            if "(" in stmt and ")" in stmt:
                base_cls = re.sub(r"<.*>", "", cls_name).strip()
                sig = stmt
                is_static = bool(re.search(r"\bstatic\b", sig))
                name_m = re.search(r"([A-Za-z_]\w*)\s*\(", sig)
                name = name_m.group(1) if name_m else ""
                if name == base_cls:
                    args_inside = re.search(r"\((.*)\)", sig, re.DOTALL)
                    arglist = []
                    if args_inside:
                        raw = args_inside.group(1).strip()
                        depth = angle = 0
                        buf = ""
                        for ch in raw:
                            if ch == '<': angle += 1
                            elif ch == '>': angle = max(0, angle - 1)
                            elif ch == '(': depth += 1
                            elif ch == ')': depth = max(0, depth - 1)
                            if ch == ',' and depth == 0 and angle == 0:
                                if buf.strip():
                                    arglist.append(buf.strip())
                                buf = ""
                            else:
                                buf += ch
                        if buf.strip():
                            arglist.append(buf.strip())
                    ctors.append(CtorInfo(args=arglist if arglist != ['void'] else []))
                else:
                    if name and not name.startswith("operator") and not name.startswith("~"):
                        methods.append(MethodInfo(name=name, is_static=is_static))
            else:
                m2 = re.search(r"([A-Za-z_]\w*)\s*$", stmt)
                if not m2:
                    continue
                fname = m2.group(1)
                ftype = stmt[:m2.start()].strip()
                ftype = re.sub(r"\b(?:mutable|constexpr|inline|volatile)\b", "", ftype).strip()
                if fname and ftype and not ftype.endswith(")"):
                    if " static " in f" {stmt} ":
                        continue
                    fields.append(FieldInfo(name=fname, type=ftype))

    return fields, methods, ctors

def regex_parse_file(path: Path, text: str) -> List[ClassInfo]:
    out: List[ClassInfo] = []
    for m in CLASS_RE.finditer(text):
        is_templ = bool(m.group("templ"))
        name = m.group("name").strip()
        open_brace = m.end() - 1
        close_brace = find_matching_brace(text, open_brace)
        after = close_brace + 1
        while after < len(text) and text[after] in " \t\r\n":
            after += 1
        if after < len(text) and text[after] == ';':
            after += 1
        body = text[open_brace + 1: close_brace]
        fields, methods, ctors = parse_public_api_from_body(body, name)
        name_token = re.sub(r"<.*>", "", name).strip()
        out.append(ClassInfo(
            name=name,
            name_token=name_token,
            start=m.start(),
            close_brace=close_brace,
            after_semicolon=after,
            is_template=is_templ,
            fields=fields,
            methods=methods,
            ctors=ctors,
        ))
    return out

# ------------------------ Block generation & insertion ------------------------
BEGIN_END = [
    ("PTX_BEGIN_FIELDS", "PTX_END_FIELDS"),
    ("PTX_BEGIN_METHODS", "PTX_END_METHODS"),
    ("PTX_BEGIN_DESCRIBE", "PTX_END_DESCRIBE"),
]

def gen_blocks(ci: ClassInfo) -> str:
    cls = ci.name_token

    # Fields
    field_lines = []
    for f in ci.fields:
        mn, mx = guess_range(f.type)
        doc = nice_doc_from_name(f.name)
        field_lines.append(f'PTX_FIELD({cls}, {f.name}, "{doc}", {mn}, {mx}),')
    if field_lines:
        field_lines[-1] = field_lines[-1].rstrip(',')

    # Methods
    method_lines = []
    for m in ci.methods:
        doc = nice_doc_from_name(m.name)
        if m.is_static:
            method_lines.append(f'PTX_SMETHOD_AUTO({cls}::{m.name}, "{doc}"),')
        else:
            method_lines.append(f'PTX_METHOD_AUTO({cls}, {m.name}, "{doc}"),')
    if method_lines:
        method_lines[-1] = method_lines[-1].rstrip(',')

    # Ctors
    ctor_lines = []
    ctor0_done = False
    for c in ci.ctors:
        if len(c.args) == 0:
            if not ctor0_done:
                ctor_lines.append(f'PTX_CTOR0({cls}),')
                ctor0_done = True
        else:
            args = ", ".join(c.args)
            ctor_lines.append(f'PTX_CTOR({cls}, {args}),')
    if ctor_lines:
        ctor_lines[-1] = ctor_lines[-1].rstrip(',')

    if not field_lines:
        field_lines = [f'/* TODO: PTX_FIELD({cls}, member, "Doc", min, max) */']
    if not method_lines:
        method_lines = [f'/* TODO: PTX_METHOD_AUTO({cls}, Method, "Doc") */']
    if not ctor_lines:
        ctor_lines = [f'/* TODO: PTX_CTOR0({cls}) or PTX_CTOR({cls}, ...) */']

    parts = []
    parts.append(f'PTX_BEGIN_FIELDS({cls})')
    parts.extend([f'    {ln}' for ln in field_lines])
    parts.append('PTX_END_FIELDS')
    parts.append('')
    parts.append(f'PTX_BEGIN_METHODS({cls})')
    parts.extend([f'    {ln}' for ln in method_lines])
    parts.append('PTX_END_METHODS')
    parts.append('')
    parts.append(f'PTX_BEGIN_DESCRIBE({cls})')
    parts.extend([f'    {ln}' for ln in ctor_lines])
    parts.append(f'PTX_END_DESCRIBE({cls})')
    parts.append('')
    
    if ci.is_template:
        parts.insert(0, f'/* NOTE: {ci.name} is a template; verify macros accept template types. */')
    return "\n".join(parts)

def find_existing_block(src: str, macro: str, cls_token: str, with_class_in_end: bool) -> Optional[re.Match]:
    if with_class_in_end:
        end_macro = macro.replace("BEGIN", "END")
        pat = rf'{macro}\(\s*{re.escape(cls_token)}\s*\)(?P<body>.*?)\s*{end_macro}\(\s*{re.escape(cls_token)}\s*\)'
    else:
        end_macro = macro.replace("BEGIN", "END")
        pat = rf'{macro}\(\s*{re.escape(cls_token)}\s*\)(?P<body>.*?)\s*{end_macro}'
    return re.search(pat, src, re.DOTALL)

def ensure_blocks_in_file(src: str, ci: ClassInfo) -> Tuple[str, bool, List[str]]:
    changed = False
    cls = ci.name_token

    has_fields = find_existing_block(src, "PTX_BEGIN_FIELDS", cls, with_class_in_end=False) is not None
    has_methods = find_existing_block(src, "PTX_BEGIN_METHODS", cls, with_class_in_end=False) is not None
    has_desc = find_existing_block(src, "PTX_BEGIN_DESCRIBE", cls, with_class_in_end=True) is not None

    if not (has_fields and has_methods and has_desc):
        block_text = gen_blocks(ci)

        line_start = src.rfind("\n", 0, ci.close_brace) + 1
        base_indent = re.match(r"[ \t]*", src[line_start:ci.close_brace]).group(0)
        inner = base_indent + "    "
        indented = inner + block_text.replace("\n", "\n" + inner) + "\n"

        src = src[:ci.close_brace] + "\n" + indented + src[ci.close_brace:]

        changed = True
        missing = []
        if not has_fields: missing.append("FIELDS")
        if not has_methods: missing.append("METHODS")
        if not has_desc: missing.append("DESCRIBE")
        print(f"Inserted {', '.join(missing)} for {ci.name}.")
        return src, changed

    m_fields = find_existing_block(src, "PTX_BEGIN_FIELDS", cls, False)
    m_methods = find_existing_block(src, "PTX_BEGIN_METHODS", cls, False)
    m_desc = find_existing_block(src, "PTX_BEGIN_DESCRIBE", cls, True)
    spans = sorted([(m_fields.start(), "FIELDS", m_fields.group(0)),
                    (m_methods.start(), "METHODS", m_methods.group(0)),
                    (m_desc.start(), "DESCRIBE", m_desc.group(0))], key=lambda x: x[0])
    order = [x[1] for x in spans]
    if order != ["FIELDS", "METHODS", "DESCRIBE"]:
        for m in sorted([m_fields, m_methods, m_desc], key=lambda m: m.start(), reverse=True):
            src = src[:m.start()] + src[m.end():]
        rein = "\n\n".join([s[2] for s in sorted(spans, key=lambda x: ["FIELDS","METHODS","DESCRIBE"].index(x[1]))]) + "\n"
        src = src[:ci.after_semicolon] + "\n\n" + rein + src[ci.after_semicolon:]
        changed = True
        print(f"Reordered PTX blocks for {ci.name}.")
    return src, changed

# ------------------------ Main ------------------------
def main():
    ap = argparse.ArgumentParser(description="Generate/repair PTX reflection blocks from headers")
    ap.add_argument("--root", default="lib/ptx", help="Root dir to scan (default: lib/ptx)")
    ap.add_argument("--write", action="store_true", help="Write changes back (creates .bak)")
    ap.add_argument("--only", nargs="*", help="Only process these class names (base name or template name)")
    ap.add_argument("--clang-arg", action="append", default=[], help="Extra arg for libclang (e.g. -Iinc -std=c++17)")
    ap.add_argument('--no-backup', action='store_true', help='Skip creating .bak backups when writing in-place')
    ap.add_argument('--exclude-dir', action='append', default=[],help='Folder name(s) to exclude, pass multiple times, e.g. --exclude-dir physics --exclude-dir render')

    args = ap.parse_args()

    root = Path(args.root)
    exclude_dirs = {d for d in (args.exclude_dir or []) if d}

    def is_excluded(path: Path) -> bool:
        return any(seg in exclude_dirs for seg in path.parts[:-1])

    headers = [p for p in root.rglob('*.hpp') if not is_excluded(p)]
    texts = {p: p.read_text(encoding="utf-8", errors="ignore") for p in headers}
    clang_results = try_clang_parse(headers, args.clang_arg)
    make_backup = not args.no_backup

    total_changed = 0
    total_files = 0

    for p in headers:
        src = texts[p]

        if p in clang_results:
            classes = clang_results[p]
        else:
            classes = regex_parse_file(p, src)

        if args.only:
            onlyset = set(args.only)
            classes = [ci for ci in classes if (ci.name_token in onlyset or ci.name in onlyset)]

        if not classes:
            continue

        classes_sorted = sorted(classes, key=lambda c: c.after_semicolon, reverse=True)
        changed_any = False
        for ci in classes_sorted:
            src, changed = ensure_blocks_in_file(src, ci)
            changed_any = changed_any or changed

        uses_macros = re.search(r'\bPTX_BEGIN_(FIELDS|METHODS|DESCRIBE)\s*\(', src) is not None
        if uses_macros:
            src, inc_changed, rel = ensure_reflect_include(src, header_path=p, root_path=Path(args.root))
            if inc_changed:
                changed_any = True
                print(f"{p}: inserted #include \"{rel}\"")

        if changed_any and args.write:
            if make_backup:
                bak = p.with_suffix(p.suffix + ".bak")
                if not bak.exists():
                    bak.write_text(texts[p], encoding="utf-8")
            p.write_text(src, encoding="utf-8")
            total_changed += 1
        total_files += 1

    print(f"Processed {total_files} headers under {root} - modified {total_changed}.")
    if not clang_results:
        print("(libclang not available or failed - used regex fallback)", file=sys.stderr)

if __name__ == "__main__":
    main()
