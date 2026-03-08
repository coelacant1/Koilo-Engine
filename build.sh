#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
CONFIG=${CONFIG:-Release}

# Auto-detect Ninja for faster builds, fall back to Make
if command -v ninja &> /dev/null && [ -z "${GENERATOR:-}" ]; then
  GENERATOR="Ninja"
else
  GENERATOR=${GENERATOR:-Unix Makefiles}
fi

# Detect number of CPU cores for parallel compilation
if [ "$(uname)" = "Linux" ]; then
  NUM_CORES=$(nproc)
elif [ "$(uname)" = "Darwin" ]; then
  NUM_CORES=$(sysctl -n hw.ncpu)
else
  NUM_CORES=4
fi

# Allow override via environment
NUM_JOBS=${NUM_JOBS:-$NUM_CORES}

# Enable ccache if available for faster rebuilds
if command -v ccache &> /dev/null && [ -z "${CMAKE_CXX_COMPILER_LAUNCHER:-}" ]; then
  export CMAKE_CXX_COMPILER_LAUNCHER=ccache
  export CMAKE_C_COMPILER_LAUNCHER=ccache
fi

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

usage() {
  cat <<EOF
KoiloEngine unified build helper
Usage: $0 [command]
Commands:
  configure         Run CMake configure step
  registry          Regenerate reflection umbrella and registry cache
  test-skeletons    Generate test skeletons for all untested classes
  namespace-check   Check for namespace violations (read-only)
  core              Build koilo_core
  reflect           Build koilo_reflect (implies generation)
  tests             Build and run all C++ tests (Unity + script language tests)
  headercheck       Build header smoke tests (compile all headers individually)
  all               Build all default targets (includes namespace check)
  clean             Remove build directory
  ctest             Run ctest in build directory
Env vars:
  CONFIG=Debug|Release (default Release)
  GENERATOR=... CMake generator name (auto-detect Ninja or Unix Makefiles)
  NUM_JOBS=N Number of parallel jobs (default: detected CPU cores: ${NUM_CORES})
  SKIP_NAMESPACE_CHECK=1  Skip namespace validation
  CMAKE_CXX_COMPILER_LAUNCHER=ccache  Use ccache (auto-detected if available)

Current settings:
  Generator: ${GENERATOR}
  Parallel jobs: ${NUM_JOBS}
  Compiler launcher: ${CMAKE_CXX_COMPILER_LAUNCHER:-none}
EOF
}

check_namespace() {
  if [ "${SKIP_NAMESPACE_CHECK:-0}" = "1" ]; then
    echo -e "${YELLOW}Skipping namespace check (SKIP_NAMESPACE_CHECK=1)${NC}"
    return 0
  fi
  
  echo -e "${GREEN}Checking namespace usage...${NC}"
  if python3 "${ROOT_DIR}/scripts/checknamespace.py" --check > /dev/null 2>&1; then
    echo -e "${GREEN}All classes use koilo namespace${NC}"
    return 0
  else
    echo -e "${RED}Namespace violations found!${NC}"
    echo -e "${YELLOW}Run: python3 scripts/checknamespace.py --check${NC}"
    echo -e "${YELLOW}To see violations in detail.${NC}"
    echo ""
    echo -e "${YELLOW}To build anyway, use: SKIP_NAMESPACE_CHECK=1 ./build.sh${NC}"
    return 1
  fi
}

cmake_configure() {
  mkdir -p "${BUILD_DIR}"
  cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -G "${GENERATOR}" -DCMAKE_BUILD_TYPE="${CONFIG}" "$@"
}

cmake_build() {
  # Use parallel compilation with detected core count
  if [ "$GENERATOR" = "Ninja" ]; then
    # Ninja handles parallelism automatically based on load
    cmake --build "${BUILD_DIR}" --config "${CONFIG}" --target "$@"
  else
    # Make needs explicit -j flag
    cmake --build "${BUILD_DIR}" --config "${CONFIG}" --parallel "${NUM_JOBS}" --target "$@"
  fi
}

cmd=${1:-all}
shift || true

case "$cmd" in
  configure)
    check_namespace
    cmake_configure "$@" ;;
  registry)
    check_namespace
    cmake_configure ; cmake_build koilo_update_registry ;;
  test-skeletons)
    cmake_configure ; cmake_build koilo_generate_test_skeletons ;;
  namespace-check)
    python3 "${ROOT_DIR}/scripts/checknamespace.py" --check ;;
  core)
    check_namespace
    cmake_configure ; cmake_build koilo_core ;;
  reflect)
    check_namespace
    cmake_configure ; cmake_build koilo_reflect ;;
  tests)
    check_namespace
    cmake_configure -DKL_BUILD_TESTS=ON ; cmake_build koilo_tests koilo_script_language_tests ; ctest --test-dir "${BUILD_DIR}" --output-on-failure ;;
  headercheck)
    check_namespace
    cmake_configure ; cmake_build koilo_headercheck ;;
  ctest)
    ctest --test-dir "${BUILD_DIR}" --output-on-failure ;;
  all)
    check_namespace
    if [ "$GENERATOR" = "Ninja" ]; then
      cmake_configure ; cmake --build "${BUILD_DIR}" --config "${CONFIG}"
    else
      cmake_configure ; cmake --build "${BUILD_DIR}" --config "${CONFIG}" --parallel "${NUM_JOBS}"
    fi ;;
  clean)
    rm -rf "${BUILD_DIR}" ;;
  *)
    usage; exit 1 ;;
esac
