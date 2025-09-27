# UpdatePTXRegistry.py - auto-generate PTX reflection blocks from C++ headers
#
# Process:
# - Walk all .hpp under a root directory (default: lib/ptx)
# - Parse classes (normal + templates) and their public API:
#     * public non-static fields  -> PTX_FIELD
#     * public methods            -> PTX_METHOD_AUTO / PTX_SMETHOD_AUTO
#     * constructors              -> PTX_CTOR / PTX_CTOR0
# - Insert blocks after each class definition (or reorder/fix if present)
# - Creates .bak backups when writing
#
# Parsing:
# - Preferred: libclang, pass extra compile args via --clang-arg.
# - Fallback: regex parser that understands 'template<...> class/struct ... { ... };'
#
# Usage examples:
#   python UpdatePTXRegistry.py --root ./lib/ptx
#   python UpdatePTXRegistry.py --root ./lib/ptx --write
#   python UpdatePTXRegistry.py --root ./lib/ptx --write --force
#

import argparse
import bisect
import re
import sys
import os
import json
import hashlib
from dataclasses import dataclass, field, asdict
from pathlib import Path
from typing import List, Optional, Tuple, Dict, Any

# Static helper names injected by the reflection macros
REFLECTION_HELPER_NAMES = {"Fields", "Methods", "Describe"}

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

PTX_BLOCK_ANY_RE = re.compile(
    r'[\t ]*PTX_BEGIN_(?:FIELDS|METHODS|DESCRIBE)\s*\([^\n]*?\)'  # begin macro line
    r'(?:\r?\n(?:.|\r?\n)*?)?'                                    # body up to
    r'PTX_END_(?:FIELDS|METHODS|DESCRIBE)(?:\s*\([^\n]*?\))?'     # matching end macro
    r'[\t ]*(?:\r?\n|)',
    re.DOTALL,
)

def strip_ptx_blocks(text: str) -> str:
    prev = None
    cur = text
    while prev != cur:
        prev = cur
        cur = PTX_BLOCK_ANY_RE.sub("\n", cur, count=1)
    cur = re.sub(r'(?:\r?\n){3,}', '\n\n', cur)
    return cur

def find_comment_spans(src: str) -> Tuple[List[int], List[Tuple[int, int]]]:
    spans: List[Tuple[int, int]] = []
    n = len(src)
    i = 0
    in_string = False
    in_char = False

    while i < n:
        ch = src[i]
        nxt = src[i + 1] if i + 1 < n else ''

        if not in_string and not in_char:
            if ch == '"':
                in_string = True
                i += 1
                continue
            if ch == "'":
                in_char = True
                i += 1
                continue
            if ch == '/' and nxt == '/':
                start = i
                i = src.find('\n', i)
                if i == -1:
                    spans.append((start, n))
                    break
                spans.append((start, i))
                continue
            if ch == '/' and nxt == '*':
                start = i
                end = src.find('*/', i + 2)
                if end == -1:
                    spans.append((start, n))
                    break
                spans.append((start, end + 2))
                i = end + 2
                continue
        else:
            if ch == '\\':
                i += 2
                continue
            if in_string and ch == '"':
                in_string = False
                i += 1
                continue
            if in_char and ch == "'":
                in_char = False
                i += 1
                continue

        i += 1

    spans.sort()
    starts = [s for s, _ in spans]
    return starts, spans


def in_comment(idx: int, starts: List[int], spans: List[Tuple[int, int]]) -> bool:
    if not spans:
        return False
    pos = bisect.bisect_right(starts, idx) - 1
    if pos >= 0 and spans[pos][0] <= idx < spans[pos][1]:
        return True
    return False

def ensure_reflect_include(src: str, header_path: Path, root_path: Path) -> tuple[str, bool, str]:
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


def _split_args(raw: str) -> List[str]:
    args: List[str] = []
    buf = ""
    depth_paren = depth_angle = depth_brace = 0
    i = 0
    while i < len(raw):
        ch = raw[i]
        if ch == '<' and depth_paren == 0:
            depth_angle += 1
        elif ch == '>' and depth_paren == 0 and depth_angle > 0:
            depth_angle -= 1
        elif ch == '(':
            depth_paren += 1
        elif ch == ')' and depth_paren > 0:
            depth_paren -= 1
        elif ch == '{' and depth_paren == 0:
            depth_brace += 1
        elif ch == '}' and depth_paren == 0 and depth_brace > 0:
            depth_brace -= 1
        if ch == ',' and depth_paren == 0 and depth_angle == 0 and depth_brace == 0:
            if buf.strip():
                args.append(buf.strip())
            buf = ""
        else:
            buf += ch
        i += 1
    if buf.strip():
        args.append(buf.strip())
    return args


def _strip_default(expr: str) -> str:
    depth_paren = depth_angle = depth_brace = 0
    for i, ch in enumerate(expr):
        if ch == '<':
            depth_angle += 1
        elif ch == '>' and depth_angle > 0:
            depth_angle -= 1
        elif ch == '(':
            depth_paren += 1
        elif ch == ')' and depth_paren > 0:
            depth_paren -= 1
        elif ch == '{':
            depth_brace += 1
        elif ch == '}' and depth_brace > 0:
            depth_brace -= 1
        elif ch == '=' and depth_paren == 0 and depth_angle == 0 and depth_brace == 0:
            return expr[:i].strip()
    return expr.strip()


def _strip_leading_attributes(expr: str) -> str:
    out = expr.strip()
    while out.startswith('[['):
        end = out.find(']]')
        if end == -1:
            break
        out = out[end + 2 :].lstrip()
    return out


KEYWORDS_TO_DROP = re.compile(r"\b(?:inline|constexpr|virtual|explicit|friend|static|noexcept|override|final|PTX_API|PTX_EXPORT|PTX_PUBLIC)\b")


def _clean_return_type(prefix: str) -> str:
    cleaned = KEYWORDS_TO_DROP.sub("", prefix)
    cleaned = re.sub(r"\s+", " ", cleaned).strip()
    return cleaned


def _normalize_param_type(param: str) -> Tuple[str, bool]:
    text = _strip_default(param)
    text = _strip_leading_attributes(text)
    if not text:
        return text, False
    if text == '...':
        return text, True
    # Remove trailing parameter name if present
    m = re.match(r"(.+?)\s+((?:\w|::)+)$", text)
    if m:
        type_part = m.group(1).strip()
        return type_part, True
    # Handle parameter pack like "Args&&... args"
    m_pack = re.match(r"(.+?\.\.\.)\s+((?:\w|::)+)$", text)
    if m_pack:
        type_part = m_pack.group(1).strip()
        return type_part, True
    # Could not confidently strip name
    return text, False

# ------------------------ Data models ------------------------
@dataclass
class FieldInfo:
    name: str
    type: str

@dataclass
class MethodInfo:
    name: str
    is_static: bool
    return_type: Optional[str] = None
    args: List[str] = field(default_factory=list)
    is_const: bool = False
    signature: str = ""
    parsed: bool = True

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
    in_template_scope: bool
    fields: List[FieldInfo] = field(default_factory=list)
    methods: List[MethodInfo] = field(default_factory=list)
    ctors: List[CtorInfo] = field(default_factory=list)
    has_pure_virtual: bool = False

def try_clang_parse(paths: List[Path], clang_args: List[str]) -> Dict[Path, List[ClassInfo]]:
    try:
        from clang import cindex
    except Exception:
        print("[clang] WARNING: libclang not available; falling back to regex parser.")
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

        def in_template_scope(cur) -> bool:
            parent = getattr(cur, "semantic_parent", None)
            while parent is not None:
                kind = getattr(parent, "kind", None)
                if kind in (
                    cindex.CursorKind.CLASS_TEMPLATE,
                    cindex.CursorKind.FUNCTION_TEMPLATE,
                    cindex.CursorKind.CLASS_TEMPLATE_PARTIAL_SPECIALIZATION,
                    cindex.CursorKind.TEMPLATE_TYPE_PARAMETER,
                ):
                    return True
                parent = getattr(parent, "semantic_parent", None)
            return False

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
                has_pure_virtual = getattr(c, "is_abstract_record", lambda: False)()

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
                        if ch.is_static_method() and nm in REFLECTION_HELPER_NAMES:
                            continue
                        arg_types = [arg.type.spelling for arg in ch.get_arguments()]
                        methods.append(MethodInfo(
                            name=nm,
                            is_static=ch.is_static_method(),
                            return_type=ch.result_type.spelling if ch.result_type else None,
                            args=arg_types,
                            is_const=ch.is_const_method(),
                            signature=ch.displayname or nm,
                            parsed=True,
                        ))
                        if not has_pure_virtual and getattr(ch, "is_pure_virtual_method", None):
                            try:
                                if ch.is_pure_virtual_method():
                                    has_pure_virtual = True
                            except Exception:
                                pass
                    elif ch.kind == cindex.CursorKind.CONSTRUCTOR:
                        arg_types = [arg.type.spelling for arg in ch.get_arguments()]
                        ctors.append(CtorInfo(args=arg_types))

                is_templ = (c.kind == cindex.CursorKind.CLASS_TEMPLATE)
                templ_scope = in_template_scope(c)
                name_text = c.spelling
                name_token = c.spelling

                out_list.append(ClassInfo(
                    name=name_text,
                    name_token=name_token,
                    start=start,
                    close_brace=close_brace if close_brace != -1 else end,
                    after_semicolon=end,
                    is_template=is_templ,
                    in_template_scope=templ_scope,
                    fields=fields,
                    methods=methods,
                    ctors=ctors,
                    has_pure_virtual=has_pure_virtual
                ))

                visit(c)

        visit(tu.cursor)
        if out_list:
            results[p] = out_list

            print(f"{pcount} of {len(paths)} - Parsing clang of {p}")
        else:
            print(f"{pcount} of {len(paths)} - Skipping {p} does not contain a class")
        
        pcount += 1

    return results

# ------------------------ Caching ------------------------
CACHE_VERSION = 1

def _fingerprint(path: Path) -> Dict[str, Any]:
    """Create a cheap fingerprint for a file combining size, mtime and a fast hash.
    The hash is only over the first + last 4KB to keep it quick for large headers.
    """
    st = path.stat()
    size = st.st_size
    mtime = int(st.st_mtime)
    h = hashlib.sha1()
    try:
        with path.open('rb') as fp:
            head = fp.read(4096)
            if size > 8192:
                fp.seek(-4096, os.SEEK_END)
            tail = fp.read(4096)
        h.update(head)
        h.update(tail)
    except Exception:
        # Fallback to just mtime/size
        pass
    return {"size": size, "mtime": mtime, "h": h.hexdigest()}

def _same_fingerprint(a: Dict[str, Any], b: Dict[str, Any]) -> bool:
    return a.get("size") == b.get("size") and a.get("mtime") == b.get("mtime") and a.get("h") == b.get("h")

def classinfo_to_dict(ci: ClassInfo) -> Dict[str, Any]:
    return {
        "name": ci.name,
        "name_token": ci.name_token,
        "start": ci.start,
        "close_brace": ci.close_brace,
        "after_semicolon": ci.after_semicolon,
        "is_template": ci.is_template,
        "in_template_scope": ci.in_template_scope,
        "has_pure_virtual": ci.has_pure_virtual,
        "fields": [asdict(f) for f in ci.fields],
        "methods": [asdict(m) for m in ci.methods],
        "ctors": [asdict(c) for c in ci.ctors],
    }

def dict_to_classinfo(d: Dict[str, Any]) -> ClassInfo:
    return ClassInfo(
        name=d["name"],
        name_token=d["name_token"],
        start=d.get("start", 0),
        close_brace=d.get("close_brace", 0),
        after_semicolon=d.get("after_semicolon", 0),
        is_template=d.get("is_template", False),
        in_template_scope=d.get("in_template_scope", False),
        fields=[FieldInfo(**f) for f in d.get("fields", [])],
        methods=[MethodInfo(**m) for m in d.get("methods", [])],
        ctors=[CtorInfo(**c) for c in d.get("ctors", [])],
        has_pure_virtual=d.get("has_pure_virtual", False),
    )

def load_cache(cache_path: Path) -> Dict[str, Any]:
    if not cache_path.exists():
        return {"version": CACHE_VERSION, "meta": {}, "files": {}}
    try:
        data = json.loads(cache_path.read_text(encoding="utf-8"))
        if data.get("version") != CACHE_VERSION:
            return {"version": CACHE_VERSION, "meta": {}, "files": {}}
        if "files" not in data:
            data["files"] = {}
        if "meta" not in data:
            data["meta"] = {}
        return data
    except Exception:
        return {"version": CACHE_VERSION, "meta": {}, "files": {}}

def save_cache(cache_path: Path, cache: Dict[str, Any]) -> None:
    try:
        tmp = cache_path.with_suffix(".tmp")
        tmp.write_text(json.dumps(cache, indent=2, sort_keys=True), encoding="utf-8")
        tmp.replace(cache_path)
    except Exception as exc:
        print(f"[cache] WARNING: failed to write cache {cache_path}: {exc}")

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

def parse_public_api_from_body(body: str, cls_name: str) -> Tuple[List[FieldInfo], List[MethodInfo], List[CtorInfo], bool]:
    fields: List[FieldInfo] = []
    methods: List[MethodInfo] = []
    ctors: List[CtorInfo] = []
    has_pure_virtual = False

    comment_starts_body, comment_spans_body = find_comment_spans(body)

    sections = []
    last = 0
    for m in ACCESS_RE.finditer(body):
        if in_comment(m.start(), comment_starts_body, comment_spans_body):
            continue
        sections.append((last, m.start(), "implicit"))
        sections.append((m.end(), None, m.group(1)))
        last = m.end()
    sections.append((last, None, "implicit"))

    public_spans: List[Tuple[int, int]] = []
    for i in range(0, len(sections), 2):
        start_idx, end_idx, kind = sections[i]
        if end_idx is None:
            end_idx = len(body)
        if i + 1 < len(sections):
            nxt = sections[i + 1]
            access_word = nxt[2]
            next_m = ACCESS_RE.search(body, nxt[0])
            nxt_end = next_m.start() if next_m else len(body)
            if access_word == "public":
                public_spans.append((nxt[0], nxt_end))

    if not public_spans:
        public_spans = [(0, len(body))]

    decl_re = re.compile(r";")
    pending_template = False

    for (s, e) in public_spans:
        segment = body[s:e]
        idx = 0
        while True:
            m = decl_re.search(segment, idx)
            if not m:
                break
            stmt_start_rel = idx
            stmt_end_rel = m.start()
            raw_stmt = segment[stmt_start_rel:stmt_end_rel]
            idx = m.end()

            stmt = raw_stmt.strip()
            if not stmt:
                continue

            leading_ws = len(raw_stmt) - len(raw_stmt.lstrip())
            stmt_abs = s + stmt_start_rel + leading_ws

            if in_comment(stmt_abs, comment_starts_body, comment_spans_body):
                continue
            if stmt.startswith(('/*', '//', '*')):
                continue
            if re.match(r"^\s*(public|private|protected)\s*:$", stmt):
                continue
            if re.match(r"^\s*(class|struct|enum)\b", stmt):
                continue

            if stmt.startswith("template"):
                pending_template = True
                continue

            if pending_template:
                if "(" in stmt and ")" in stmt:
                    pending_template = False
                    continue
                # any other statement clears the flag as well
                pending_template = False

            if "template<" in stmt:
                continue

            if "(" in stmt and ")" in stmt:
                base_cls = re.sub(r"<.*>", "", cls_name).strip()
                sig = stmt
                is_static = bool(re.search(r"\bstatic\b", sig))
                name_m = re.search(r"([A-Za-z_]\w*)\s*\(", sig)
                name = name_m.group(1) if name_m else ""
                if name == base_cls:
                    args_inside = re.search(r"\((.*)\)", sig, re.DOTALL)
                    arglist: List[str] = []
                    if args_inside:
                        raw = args_inside.group(1).strip()
                        depth = angle = 0
                        buf = ""
                        for ch in raw:
                            if ch == '<':
                                angle += 1
                            elif ch == '>':
                                angle = max(0, angle - 1)
                            elif ch == '(':
                                depth += 1
                            elif ch == ')':
                                depth = max(0, depth - 1)
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
                        if is_static and name in REFLECTION_HELPER_NAMES:
                            continue
                        args_inside = re.search(r"\((.*)\)", sig, re.DOTALL)
                        arg_types: List[str] = []
                        args_ok = True
                        if args_inside:
                            raw_args = args_inside.group(1).strip()
                            if raw_args and raw_args != 'void':
                                for part in _split_args(raw_args):
                                    arg_type, ok = _normalize_param_type(part)
                                    arg_types.append(arg_type)
                                    args_ok = args_ok and ok
                        is_const_method = bool(re.search(r"\)\s*const\b", sig))

                        ret_type: Optional[str] = None
                        parsed_ok = True
                        arrow_pos = sig.find('->')
                        if arrow_pos != -1:
                            tail = sig[arrow_pos + 2 :].strip()
                            tail = re.split(r"\b(?:noexcept|override|final)\b", tail)[0].strip()
                            tail = tail.rstrip(';').strip()
                            ret_type = tail or None
                        elif name_m:
                            prefix = sig[:name_m.start()].strip()
                            ret_type = _clean_return_type(prefix) or None
                        if not ret_type:
                            parsed_ok = False
                        pure_virtual = bool(re.search(r'=\s*0\s*$', sig))
                        if pure_virtual:
                            has_pure_virtual = True
                        methods.append(MethodInfo(
                            name=name,
                            is_static=is_static,
                            return_type=ret_type,
                            args=arg_types,
                            is_const=is_const_method,
                            signature=sig.strip(),
                            parsed=parsed_ok and args_ok,
                        ))
                continue

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

    return fields, methods, ctors, has_pure_virtual

def validate_macro_blocks(src: str) -> None:
    starts, spans = find_comment_spans(src)

    begin_patterns: Dict[str, re.Pattern] = {
        'FIELDS': re.compile(r'PTX_BEGIN_FIELDS\(\s*([^\)]+?)\s*\)'),
        'METHODS': re.compile(r'PTX_BEGIN_METHODS\(\s*([^\)]+?)\s*\)'),
        'DESCRIBE': re.compile(r'PTX_BEGIN_DESCRIBE\(\s*([^\)]+?)\s*\)'),
    }
    end_patterns: Dict[str, re.Pattern] = {
        'FIELDS': re.compile(r'PTX_END_FIELDS'),
        'METHODS': re.compile(r'PTX_END_METHODS'),
        'DESCRIBE': re.compile(r'PTX_END_DESCRIBE'),
    }

    begin_counts: Dict[Tuple[str, str], int] = {}
    total_begin: Dict[str, int] = {k: 0 for k in begin_patterns}
    total_end: Dict[str, int] = {k: 0 for k in end_patterns}

    for kind, pattern in begin_patterns.items():
        for match in pattern.finditer(src):
            if in_comment(match.start(), starts, spans):
                continue
            cls = match.group(1).strip()
            key = (cls, kind)
            begin_counts[key] = begin_counts.get(key, 0) + 1
            total_begin[kind] += 1

    for kind, pattern in end_patterns.items():
        for match in pattern.finditer(src):
            if in_comment(match.start(), starts, spans):
                continue
            total_end[kind] += 1

    duplicates = [(cls, kind, count) for (cls, kind), count in begin_counts.items() if count > 1]
    mismatched = [kind for kind in total_begin if total_begin[kind] != total_end[kind]]

    if duplicates or mismatched:
        messages = []
        if duplicates:
            for cls, kind, count in duplicates:
                messages.append(f"Duplicate PTX_{kind} block for {cls} ({count} occurrences)")
        if mismatched:
            for kind in mismatched:
                messages.append(f"Mismatched PTX_{kind} blocks: begins={total_begin[kind]}, ends={total_end[kind]}")
        raise RuntimeError("Invalid PTX macro blocks detected:\n" + "\n".join(messages))


def regex_parse_file(path: Path, text: str) -> List[ClassInfo]:
    out: List[ClassInfo] = []
    comment_starts, comment_spans = find_comment_spans(text)
    for m in CLASS_RE.finditer(text):
        if in_comment(m.start(), comment_starts, comment_spans):
            continue
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
        fields, methods, ctors, has_pure_virtual = parse_public_api_from_body(body, name)
        name_token = re.sub(r"<.*>", "", name).strip()
        out.append(ClassInfo(
            name=name,
            name_token=name_token,
            start=m.start(),
            close_brace=close_brace,
            after_semicolon=after,
            is_template=is_templ,
            in_template_scope=False,
            fields=fields,
            methods=methods,
            ctors=ctors,
            has_pure_virtual=has_pure_virtual,
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
    method_lines: List[str] = []
    method_counts: Dict[Tuple[str, bool], int] = {}
    for m in ci.methods:
        key = (m.name, m.is_static)
        method_counts[key] = method_counts.get(key, 0) + 1

    static_names = {m.name for m in ci.methods if m.is_static}
    member_names = {m.name for m in ci.methods if not m.is_static}

    def format_member_overload(m: MethodInfo) -> str:
        if not m.args:
            macro = 'PTX_METHOD_OVLD_CONST0' if m.is_const else 'PTX_METHOD_OVLD0'
            return f'{macro}({cls}, {m.name}, {m.return_type})'
        macro = 'PTX_METHOD_OVLD_CONST' if m.is_const else 'PTX_METHOD_OVLD'
        arg_list = ', '.join(m.args)
        return f'{macro}({cls}, {m.name}, {m.return_type}, {arg_list})'

    def format_static_overload(m: MethodInfo) -> str:
        if not m.args:
            return f'PTX_SMETHOD_OVLD0({cls}, {m.name}, {m.return_type})'
        arg_list = ', '.join(m.args)
        return f'PTX_SMETHOD_OVLD({cls}, {m.name}, {m.return_type}, {arg_list})'

    for m in ci.methods:
        doc = nice_doc_from_name(m.name)
        overloaded = method_counts[(m.name, m.is_static)] > 1
        conflict_with_static = (not m.is_static) and (m.name in static_names)
        doc_prefix = f'/* {doc} */ ' if doc else ''

        if m.is_static:
            if overloaded or m.name in member_names:
                if m.parsed and m.return_type:
                    method_lines.append(f'{doc_prefix}{format_static_overload(m)},')
                else:
                    method_lines.append(f'/* Unsupported static overload: {cls}::{m.name} */')
            else:
                method_lines.append(f'PTX_SMETHOD_AUTO({cls}::{m.name}, "{doc}"),')
            continue

        if overloaded or conflict_with_static:
            if m.parsed and m.return_type:
                method_lines.append(f'{doc_prefix}{format_member_overload(m)},')
            else:
                method_lines.append(f'/* Unsupported overload: {cls}::{m.name} */')
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
        field_lines = ['/* No reflected fields. */']
    if not method_lines:
        method_lines = ['/* No reflected methods. */']
    if not ctor_lines:
        ctor_lines = ['/* No reflected ctors. */']

    parts = []
    parts.append(f'PTX_BEGIN_FIELDS({cls})')
    for ln in field_lines:
        parts.append(f'    {ln}')
    parts.append('PTX_END_FIELDS')
    parts.append('')

    parts.append(f'PTX_BEGIN_METHODS({cls})')
    for ln in method_lines:
        parts.append(f'    {ln}')
    parts.append('PTX_END_METHODS')
    parts.append('')
    
    parts.append(f'PTX_BEGIN_DESCRIBE({cls})')
    for ln in ctor_lines:
        parts.append(f'    {ln}')
    parts.append(f'PTX_END_DESCRIBE({cls})')
    parts.append('')
    
    return "\n".join(parts)

def find_existing_block(src: str, macro: str, cls_token: str, with_class_in_end: bool) -> Optional[re.Match]:
    if with_class_in_end:
        end_macro = macro.replace("BEGIN", "END")
        pat = rf'{macro}\(\s*{re.escape(cls_token)}\s*\)(?P<body>.*?)\s*{end_macro}\(\s*{re.escape(cls_token)}\s*\)'
    else:
        end_macro = macro.replace("BEGIN", "END")
        pat = rf'{macro}\(\s*{re.escape(cls_token)}\s*\)(?P<body>.*?)\s*{end_macro}'
    return re.search(pat, src, re.DOTALL)

def ensure_blocks_in_file(src: str, ci: ClassInfo, force: bool = False) -> Tuple[str, bool]:
    cls = ci.name_token

    template_note_re = re.compile(r'\n?[ \t]*/\*[^*]*template[^*]*\*/\n?', re.IGNORECASE)

    def _remove_existing_blocks(text: str) -> str:
        for macro, with_end in (
            ("PTX_BEGIN_FIELDS", False),
            ("PTX_BEGIN_METHODS", False),
            ("PTX_BEGIN_DESCRIBE", True),
        ):
            while True:
                match = find_existing_block(text, macro, cls, with_class_in_end=with_end)
                if not match:
                    break
                start = match.start()
                note_match = template_note_re.search(text, 0, start)
                if note_match and note_match.end() == start:
                    start = note_match.start()
                else:
                    line_start = text.rfind('\n', 0, start)
                    if line_start != -1:
                        start = line_start + 1
                end = match.end()
                while end < len(text) and text[end] in ' \t':
                    end += 1
                removed_newline = False
                while end < len(text) and text[end] == '\n':
                    end += 1
                    removed_newline = True
                text = text[:start] + text[end:]
                if removed_newline:
                    text = re.sub(r'\n{3,}', '\n\n', text)
        text = template_note_re.sub('\n', text)
        text = re.sub(r'(?:\n[ \t]*){3,}', '\n\n', text)
        return text

    def _render_block(text: str) -> Tuple[str, bool]:
        open_brace = text.find('{', ci.start)
        if open_brace == -1:
            return text, False
        close_brace = find_matching_brace(text, open_brace)
        block_text = gen_blocks(ci)

        line_break = text.rfind("\n", 0, close_brace)
        if line_break == -1:
            prefix = text[:close_brace]
            closing_indent = ""
        else:
            prefix = text[:line_break + 1]
            closing_indent = text[line_break + 1:close_brace]

        suffix = text[close_brace:]

        prefix = prefix.rstrip()
        if prefix:
            prefix += "\n\n"

        base_indent = closing_indent + "    "
        lines = block_text.strip().split('\n')
        formatted_lines = [base_indent + ln if ln else "" for ln in lines]
        formatted = "\n".join(formatted_lines)

        new_text = prefix + formatted + "\n\n" + closing_indent + suffix
        return new_text, True

    if ci.is_template or ci.in_template_scope:
        original = src
        src = _remove_existing_blocks(src)
        src = template_note_re.sub('\n', src)
        span_start = ci.start
        span_end = ci.after_semicolon
        segment = src[span_start:span_end]
        cleaned_segment = strip_ptx_blocks(segment)
        if cleaned_segment != segment:
            src = src[:span_start] + cleaned_segment + src[span_end:]
        if original != src:
            kind = "template" if ci.is_template else "template-scope"
            print(f"Skipped {kind} class {ci.name}.")
            return src, True
        return src, False

    if ci.has_pure_virtual:
        original = src
        src = _remove_existing_blocks(src)
        if original != src:
            print(f"Skipped abstract class {ci.name} (pure virtual).")
            return src, True
        return src, False

    if force:
        src = _remove_existing_blocks(src)
        return _render_block(src)

    changed = False

    has_fields = find_existing_block(src, "PTX_BEGIN_FIELDS", cls, with_class_in_end=False) is not None
    has_methods = find_existing_block(src, "PTX_BEGIN_METHODS", cls, with_class_in_end=False) is not None
    has_desc = find_existing_block(src, "PTX_BEGIN_DESCRIBE", cls, with_class_in_end=True) is not None

    if not (has_fields and has_methods and has_desc):
        src = _remove_existing_blocks(src)
        src, inserted = _render_block(src)
        if inserted:
            missing = []
            if not has_fields: missing.append("FIELDS")
            if not has_methods: missing.append("METHODS")
            if not has_desc: missing.append("DESCRIBE")
            print(f"Inserted {', '.join(missing)} for {ci.name}.")
        return src, inserted

    m_fields = find_existing_block(src, "PTX_BEGIN_FIELDS", cls, False)
    m_methods = find_existing_block(src, "PTX_BEGIN_METHODS", cls, False)
    m_desc = find_existing_block(src, "PTX_BEGIN_DESCRIBE", cls, True)
    spans = sorted([(m_fields.start(), "FIELDS", m_fields.group(0)),
                    (m_methods.start(), "METHODS", m_methods.group(0)),
                    (m_desc.start(), "DESCRIBE", m_desc.group(0))], key=lambda x: x[0])
    order = [x[1] for x in spans]
    if order != ["FIELDS", "METHODS", "DESCRIBE"]:
        src = _remove_existing_blocks(src)
        src, inserted = _render_block(src)
        if inserted:
            print(f"Reordered PTX blocks for {ci.name}.")
        return src, inserted

    return src, changed


def remove_reflect_include(src: str) -> Tuple[str, bool]:
    m = INCLUDE_RE.search(src)
    if not m:
        return src, False
    line_start = src.rfind('\n', 0, m.start()) + 1
    line_end = src.find('\n', m.end())
    if line_end == -1:
        line_end = len(src)
    removed = src[:line_start] + src[line_end + 1:]
    return removed, removed != src

EXCLUDED_SUBTREES = {"registry", "platform"}

def main():
    ap = argparse.ArgumentParser(description="Generate/repair PTX reflection blocks from headers")
    ap.add_argument("--root", default="lib/ptx", help="Root dir to scan (default: lib/ptx)")
    ap.add_argument("--write", action="store_true", help="Write changes back in-place")
    ap.add_argument("--force", action="store_true", help="Regenerate macros even when they already exist")
    ap.add_argument("--cache", dest="use_cache", action="store_true", help="Enable parse result cache (default)")
    ap.add_argument("--no-cache", dest="use_cache", action="store_false", help="Disable parse result cache")
    ap.set_defaults(use_cache=True)
    ap.add_argument("--cache-file", default=".ptx_reflect_cache.json", help="Path to cache file (default: .ptx_reflect_cache.json in project root)")
    ap.add_argument("--reparse-all", action="store_true", help="Ignore cache entries and reparse all headers (still updates cache)")

    args = ap.parse_args()

    script_dir = Path(__file__).resolve().parent
    project_root = script_dir.parent

    root_arg = Path(args.root)
    if root_arg.is_absolute():
        root = root_arg
    else:
        candidate = (project_root / root_arg).resolve()
        if candidate.exists():
            root = candidate
        else:
            root = root_arg.resolve()
    def is_excluded(path: Path) -> bool:
        parts = Path(path).relative_to(root).parts
        return parts and parts[0] in EXCLUDED_SUBTREES

    # Gather all headers, then filter out templated and virtual-only files
    all_headers = [p for p in root.rglob('*.hpp') if not is_excluded(p)]
    skipped_templated: List[Path] = []
    skipped_virtual: List[Path] = []
    headers: List[Path] = []
    for p in all_headers:
        text = p.read_text(encoding="utf-8", errors="ignore")
        if re.search(r'\btemplate\s*<', text):
            skipped_templated.append(p)
            continue
        if re.search(r'\bvirtual\b', text):
            skipped_virtual.append(p)
            continue
        headers.append(p)
    # Report skipped counts
    print(f"[filter] skipped {len(skipped_templated)} templated files, {len(skipped_virtual)} virtual files")

    cache_path = (project_root / args.cache_file) if not Path(args.cache_file).is_absolute() else Path(args.cache_file)
    cache: Dict[str, Any] = {}
    if args.use_cache:
        cache = load_cache(cache_path)
    else:
        cache = {"version": CACHE_VERSION, "meta": {}, "files": {}}

    # Record macro include file mtime; if changed, force reparse all
    macros_file = (root / "registry" / "reflect_macros.hpp")
    macros_mtime = int(macros_file.stat().st_mtime) if macros_file.exists() else 0
    cached_macros_mtime = cache.get("meta", {}).get("macros_mtime")
    macros_changed = cached_macros_mtime != macros_mtime
    if macros_changed and args.use_cache:
        print("[cache] reflect_macros.hpp changed -> invalidating all cached headers")

    parse_headers: List[Path] = []
    reused_headers: List[Path] = []
    for h in headers:
        fp = _fingerprint(h)
        key = str(h.resolve())
        entry = cache.get("files", {}).get(key)
        if (not args.use_cache) or args.reparse_all or macros_changed or entry is None or not _same_fingerprint(fp, entry.get("fingerprint", {})):
            parse_headers.append(h)
        else:
            reused_headers.append(h)

    texts = {p: p.read_text(encoding="utf-8", errors="ignore") for p in parse_headers}

    clang_results = try_clang_parse(parse_headers, []) if parse_headers else {}

    # Build combined class map using cache for reused headers
    combined_results: Dict[Path, List[ClassInfo]] = {}

    # Newly parsed headers
    for p in parse_headers:
        if p in clang_results:
            combined_results[p] = clang_results[p]
        else:
            # fallback regex parse
            text = texts[p]
            combined_results[p] = regex_parse_file(p, text)

        # Update cache entry
        if args.use_cache:
            key = str(p.resolve())
            cache.setdefault("files", {})[key] = {
                "fingerprint": _fingerprint(p),
                "classes": [classinfo_to_dict(ci) for ci in combined_results[p]],
                "parser": "clang" if p in clang_results else "regex",
            }

    # Reused headers from cache
    reused_count_missing = 0
    for p in reused_headers:
        key = str(p.resolve())
        entry = cache.get("files", {}).get(key)
        if not entry:
            reused_count_missing += 1
            parse_headers.append(p)  # schedule for parse next run
            continue
        combined_results[p] = [dict_to_classinfo(d) for d in entry.get("classes", [])]

    # Update macro meta and store cache
    if args.use_cache:
        cache.setdefault("meta", {})["macros_mtime"] = macros_mtime
        save_cache(cache_path, cache)
        print(f"[cache] reused {len(reused_headers) - reused_count_missing} headers, reparsed {len(parse_headers)} headers")

    total_changed = 0
    total_files = 0
    force = args.force

    # We still must read source text for all headers for macro maintenance; for reused headers we lazily read now.
    for p in headers:
        if p not in texts:
            # Not parsed this run (cache reuse) -> load text now only if we might modify or write
            texts[p] = p.read_text(encoding="utf-8", errors="ignore")
        orig_src = texts[p]
        src = orig_src

        try:
            validate_macro_blocks(src)
        except RuntimeError as exc:
            raise RuntimeError(f"{p}: {exc}") from exc

        classes = combined_results.get(p, [])
        if not classes:
            continue

        classes_sorted = sorted(classes, key=lambda c: c.after_semicolon, reverse=True)
        changed_any = False
        for ci in classes_sorted:
            src, changed = ensure_blocks_in_file(src, ci, force=force)
            changed_any = changed_any or changed

        has_macros = re.search(r'\bPTX_BEGIN_(FIELDS|METHODS|DESCRIBE)\s*\(', src) is not None
        if has_macros:
            src, inc_changed, rel = ensure_reflect_include(src, header_path=p, root_path=root)
            if inc_changed:
                changed_any = True
                print(f"{p}: inserted #include \"{rel}\"")
        else:
            src, removed = remove_reflect_include(src)
            if removed:
                changed_any = True

        try:
            validate_macro_blocks(src)
        except RuntimeError as exc:
            raise RuntimeError(f"{p}: {exc}") from exc

        # Only write if actual content changed (skip identical rewrites)
        if args.write and src != orig_src:
            p.write_text(src, encoding="utf-8")
            total_changed += 1
        total_files += 1

    print(f"Processed {total_files} headers under {root} - modified {total_changed}.")
    if not clang_results and parse_headers:
        print("(libclang not available or failed for reparsed headers - used regex fallback)", file=sys.stderr)

if __name__ == "__main__":
    main()
