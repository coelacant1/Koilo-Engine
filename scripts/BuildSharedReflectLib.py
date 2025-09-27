from os.path import join
from pathlib import Path
import glob
import subprocess
import sys
import re

Import("env")

project_dir = Path(env["PROJECT_DIR"])

env.Replace(PROGNAME="ptx_reflect", PROGSUFFIX=".so")
env.AppendUnique(CCFLAGS=["-fPIC"])
env.AppendUnique(LINKFLAGS=["-shared"])

env.AppendUnique(CPPPATH=[project_dir.joinpath("lib").as_posix()])

sources = []

gen_script = project_dir.joinpath('.scripts', 'GeneratePTXAll.py')
if gen_script.exists():
    try:
        print(f"[reflect-lib] INFO: running {gen_script}")
        subprocess.run([sys.executable, str(gen_script)], check=True)
    except Exception as e:
        print(f"[reflect-lib] WARN: GeneratePTXAll.py failed: {e}")
else:
    print(f"[reflect-lib] INFO: no generator found at {gen_script}; proceeding")

ptx_include = project_dir.joinpath('lib', 'ptx')
gen_cpp = project_dir.joinpath('src', 'reflection_entry_gen.cpp')
try:
    class_names = set()
    bogus_tokens = set(['CLASS', 'INTERFACE', 'STRUCT', 'ENUM'])
    identifier_re = re.compile(r'[A-Za-z_][A-Za-z0-9_]*')

    if ptx_include.exists():
        for p in ptx_include.rglob('*.hpp'):
            try:
                txt = p.read_text(encoding='utf-8')
            except Exception:
                continue

            for m in re.finditer(r'PTX_BEGIN_DESCRIBE\s*\(\s*([A-Za-z_][A-Za-z0-9_:]*)\s*\)', txt):
                raw_name = m.group(1)
                line_start = txt.rfind('\n', 0, m.start())
                line = txt[line_start+1:m.end()]

                if line.strip().startswith('#'):
                    continue
                
                if '::' in raw_name:
                    candidate = raw_name.split('::')[-1]
                else:
                    candidate = raw_name

                if candidate.upper() in bogus_tokens:
                    continue

                if not identifier_re.fullmatch(candidate):
                    continue

                decl_re = re.compile(r'^[ \t]*(class|struct)\s+' + re.escape(candidate) + r'\b', re.M)
                mdecl = decl_re.search(txt)
                if mdecl:
                    decl_pos = mdecl.start()
                    enclosing_re = re.compile(r'\b(class|struct)\s+([A-Za-z_][A-Za-z0-9_]*)\b')
                    last_enclosing = None
                    for em in enclosing_re.finditer(txt[:decl_pos]):
                        last_enclosing = em
                    nested = False
                    if last_enclosing:
                        part = txt[last_enclosing.end():decl_pos]
                        opens = part.count('{')
                        closes = part.count('}')
                        if opens > closes:
                            nested = True
                    if nested:
                        continue

                    ns_re = re.compile(r'\bnamespace\s+([A-Za-z_][A-Za-z0-9_]*)')
                    namespaces = []
                    for nm in ns_re.finditer(txt[:decl_pos]):
                        brace_idx = txt.find('{', nm.end())
                        if brace_idx != -1 and brace_idx < decl_pos:
                            part = txt[nm.end():decl_pos]
                            if part.count('{') > part.count('}'):
                                namespaces.append(nm.group(1))
                    if namespaces:
                        qualified = '::'.join(namespaces + [candidate])
                        class_names.add(qualified)
                    else:
                        class_names.add(candidate)
                else:
                    if re.search(r'\b(class|struct)\s+' + re.escape(candidate) + r'\b', txt):
                        class_names.add(candidate)
                    else:
                        enclosing_re = re.compile(r'\b(class|struct)\s+([A-Za-z_][A-Za-z0-9_]*)\b')
                        last_enclosing = None
                        for em in enclosing_re.finditer(txt[:m.start()]):
                            last_enclosing = em.group(2)
                        if last_enclosing:
                            qualified = f"{last_enclosing}::{candidate}"
                            if re.search(r'\b' + re.escape(last_enclosing) + r'\b.*\b' + re.escape(candidate) + r'\b', txt, re.S):
                                class_names.add(qualified)
                            else:
                                class_names.add(qualified)
                        else:
                            pass
    if class_names:
        print(f"[reflect-lib] INFO: generating {gen_cpp} with {len(class_names)} Describe() calls")
        gen_cpp.parent.mkdir(parents=True, exist_ok=True)
        try:
            if gen_cpp.exists():
                gen_cpp.unlink()
        except Exception:
            pass
        with gen_cpp.open('w', encoding='utf-8') as fp:
            fp.write('#include "../lib/ptx/ptxall.hpp"\n\n')
            fp.write('namespace { struct _AutoDescribe { _AutoDescribe() {\n')
            for cls in sorted(class_names):
                fp.write(f'    (void){cls}::Describe();\n')
            fp.write('} }; static _AutoDescribe _auto_describe_instance; }\n')
        print(f"[reflect-lib] DEBUG: wrote Describe() calls for: {sorted(class_names)}")
    else:
        if gen_cpp.exists():
            try:
                gen_cpp.unlink()
            except Exception:
                pass
except Exception as e:
    print(f"[reflect-lib] WARN: failed to auto-generate reflection entry: {e}")

def safe_build_sources(build_subdir: str, src_dir: Path, include_filename: str):
    if not src_dir.exists():
        print(f"[reflect-lib] WARN: source dir not found: {src_dir}")
        return []
    
    src_path = src_dir / include_filename

    if not src_path.exists():
        print(f"[reflect-lib] WARN: source file not found: {src_path}")
        return []
    
    built = env.BuildSources(
        join("$BUILD_DIR", build_subdir),
        src_dir.as_posix(),
        f"+<{include_filename}>",
    )
    
    return list(built) if built else []

capi_dir = project_dir / "lib" / "ptx_c_api"
sources += safe_build_sources("ptx_c_api", capi_dir, "reflect_capi.cpp")

bootstrap = project_dir / "src" / "reflection_entry.cpp"
gen_bootstrap = gen_cpp
if gen_bootstrap.exists():
    sources += safe_build_sources("reflection", project_dir / "src", gen_bootstrap.name)
elif bootstrap.exists():
    sources += safe_build_sources("reflection", project_dir / "src", "reflection_entry.cpp")
else:
    print(f"[reflect-lib] INFO: no bootstrapper found at {bootstrap} or {gen_bootstrap}")

if sources:
    env.AppendUnique(SOURCES=sources)
else:
    print("[reflect-lib] ERROR: no sources were added; shared library will be empty.")

ptx_src_dir = project_dir.joinpath('lib', 'ptx')
if ptx_src_dir.exists():
    print(f"[reflect-lib] INFO: building static archive from {ptx_src_dir}")
    lib_build = env.BuildLibrary(join('$BUILD_DIR', 'libptx'), ptx_src_dir.as_posix())
    if lib_build:
        print(f"[reflect-lib] INFO: adding libptx archive to link");
        env.AppendUnique(LIBS=['libptx'])
        env.AppendUnique(LIBPATH=[join('$BUILD_DIR','libptx')])
    else:
        print('[reflect-lib] WARN: libptx build returned no artifacts')
else:
    print(f"[reflect-lib] WARN: ptx source dir not found: {ptx_src_dir}")
