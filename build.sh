#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
CONFIG=${CONFIG:-Release}
GENERATOR=${GENERATOR:-Unix Makefiles}

usage() {
  cat <<EOF
PTXEngine unified build helper
Usage: $0 [command]
Commands:
  configure         Run CMake configure step
  registry          Regenerate reflection umbrella and registry cache
  core              Build ptx_core
  reflect           Build ptx_reflect (implies generation)
  lua               Build Lua module (ptx_lua)
  tests             Build and run C++ tests
  all               Build all default targets
  clean             Remove build directory
  ctest             Run ctest in build directory
Env vars:
  CONFIG=Debug|Release (default Release)
  GENERATOR=... CMake generator name (default Unix Makefiles)
EOF
}

cmake_configure() {
  mkdir -p "${BUILD_DIR}"
  cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -G "${GENERATOR}" -DCMAKE_BUILD_TYPE="${CONFIG}" "$@"
}

cmake_build() {
  cmake --build "${BUILD_DIR}" --config "${CONFIG}" --target "$@"
}

cmd=${1:-all}
shift || true

case "$cmd" in
  configure)
    cmake_configure "$@" ;;
  registry)
    cmake_configure ; cmake_build ptx_update_registry ;;
  core)
    cmake_configure ; cmake_build ptx_core ;;
  reflect)
    cmake_configure ; cmake_build ptx_reflect ;;
  lua)
    cmake_configure -DPTX_BUILD_LUA=ON ; cmake_build ptx_lua ;;
  tests)
    cmake_configure -DPTX_BUILD_TESTS=ON ; cmake_build ptx_tests ; ctest --test-dir "${BUILD_DIR}" --output-on-failure ;;
  ctest)
    ctest --test-dir "${BUILD_DIR}" --output-on-failure ;;
  all)
    cmake_configure ; cmake --build "${BUILD_DIR}" --config "${CONFIG}" ;;
  clean)
    rm -rf "${BUILD_DIR}" ;;
  *)
    usage; exit 1 ;;
esac
