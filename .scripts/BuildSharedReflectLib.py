"""PlatformIO extra script to build the PTX reflection shared library."""

from os.path import join
from pathlib import Path

Import("env")

project_dir = Path(env["PROJECT_DIR"])

# Ensure shared-library output
env.Replace(PROGNAME="ptx_reflect", PROGSUFFIX=".so")
env.AppendUnique(CCFLAGS=["-fPIC"])
env.AppendUnique(LINKFLAGS=["-shared"])

# Make sure headers from lib/ are accessible
env.AppendUnique(CPPPATH=[project_dir.joinpath("lib").as_posix()])

sources = []

def safe_build_sources(build_subdir: str, src_dir: Path, include_filename: str):
    """Build a single file from src_dir using src_filter, returning a list (maybe empty)."""
    if not src_dir.exists():
        print(f"[reflect-lib] WARN: source dir not found: {src_dir}")
        return []
    src_path = src_dir / include_filename
    if not src_path.exists():
        print(f"[reflect-lib] WARN: source file not found: {src_path}")
        return []
    # Use src_filter syntax understood by PlatformIO/SCons: +<file>
    built = env.BuildSources(
        join("$BUILD_DIR", build_subdir),
        src_dir.as_posix(),
        f"+<{include_filename}>",
    )
    # BuildSources returns None if nothing matched; normalize to list
    return list(built) if built else []

# Build the reflection C API source explicitly
capi_dir = project_dir / "lib" / "ptx_c_api"
sources += safe_build_sources("ptx_c_api", capi_dir, "reflect_capi.cpp")

# Optionally pull in a registry bootstrapper if present
bootstrap = project_dir / "src" / "reflection_entry.cpp"
if bootstrap.exists():
    sources += safe_build_sources("reflection", project_dir / "src", "reflection_entry.cpp")
else:
    print(f"[reflect-lib] INFO: no bootstrapper found at {bootstrap}")

# Append only if we actually collected any sources
if sources:
    env.AppendUnique(SOURCES=sources)
else:
    print("[reflect-lib] ERROR: no sources were added; shared library will be empty.")
