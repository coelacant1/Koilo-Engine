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
  GENERATOR=${GENERATOR:-"Unix Makefiles"}
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
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# ---- Backend / feature flags (defaults match CMakeLists.txt) --------
VULKAN=${VULKAN:-ON}
OPENGL=${OPENGL:-ON}
AI=${AI:-ON}
AUDIO=${AUDIO:-ON}
PARTICLES=${PARTICLES:-ON}

# Parse flags from any position in the argument list
POSITIONAL_ARGS=()
for arg in "$@"; do
  case "$arg" in
    --no-vulkan)   VULKAN=OFF ;;
    --no-opengl)   OPENGL=OFF ;;
    --no-ai)       AI=OFF ;;
    --no-audio)    AUDIO=OFF ;;
    --no-particles) PARTICLES=OFF ;;
    --vulkan)      VULKAN=ON ;;
    --opengl)      OPENGL=ON ;;
    --software-only) VULKAN=OFF; OPENGL=OFF ;;
    --skip-ns)     export SKIP_NAMESPACE_CHECK=1 ;;
    --debug)       CONFIG=Debug ;;
    --release)     CONFIG=Release ;;
    --help|-h)     POSITIONAL_ARGS+=("help") ;;
    *)             POSITIONAL_ARGS+=("$arg") ;;
  esac
done
set -- "${POSITIONAL_ARGS[@]+"${POSITIONAL_ARGS[@]}"}"

usage() {
  cat <<EOF
KoiloEngine unified build helper
Usage: $(basename "$0") [command] [flags...]

Commands:
  all               Build all default targets (default command)
  configure         Run CMake configure step only
  registry          Regenerate reflection umbrella and registry cache
  test-skeletons    Generate test skeletons for all untested classes
  namespace-check   Check for namespace violations (read-only)
  core              Build koilo_core library
  reflect           Build koilo_reflect (implies generation)
  tests             Build and run all C++ tests
  headercheck       Build header smoke tests
  ctest             Run ctest in build directory
  clean             Remove build directory

Flags:
  --no-vulkan       Disable Vulkan backend
  --no-opengl       Disable OpenGL backend
  --software-only   Disable both Vulkan and OpenGL (software renderer only)
  --vulkan          Enable Vulkan backend (default)
  --opengl          Enable OpenGL backend (default)
  --no-ai           Disable AI subsystem
  --no-audio        Disable audio subsystem
  --no-particles    Disable particle subsystem
  --skip-ns         Skip namespace validation
  --debug           Build in Debug mode
  --release         Build in Release mode (default)
  -h, --help        Show this help

Environment variables:
  CONFIG=Debug|Release        Build type (default: Release)
  GENERATOR=...               CMake generator (default: auto-detect Ninja)
  NUM_JOBS=N                  Parallel jobs (default: ${NUM_CORES} cores)
  SKIP_NAMESPACE_CHECK=1      Skip namespace validation
  VULKAN=ON|OFF               Vulkan backend (default: ON)
  OPENGL=ON|OFF               OpenGL backend (default: ON)

Current settings:
  Build type:  ${CONFIG}
  Generator:   ${GENERATOR}
  Jobs:        ${NUM_JOBS}
  ccache:      ${CMAKE_CXX_COMPILER_LAUNCHER:-none}
  Vulkan:      ${VULKAN}
  OpenGL:      ${OPENGL}
  AI:          ${AI}
  Audio:       ${AUDIO}
  Particles:   ${PARTICLES}
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
    echo -e "${YELLOW}To build anyway: ./build.sh --skip-ns${NC}"
    return 1
  fi
}

cmake_configure() {
  local extra_args=("$@")
  mkdir -p "${BUILD_DIR}"
  echo -e "${CYAN}Configuring: Vulkan=${VULKAN}  OpenGL=${OPENGL}  AI=${AI}  Audio=${AUDIO}  Particles=${PARTICLES}${NC}"
  cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -G "${GENERATOR}" \
    -DCMAKE_BUILD_TYPE="${CONFIG}" \
    -DKOILO_USE_VULKAN="${VULKAN}" \
    -DKOILO_USE_OPENGL="${OPENGL}" \
    -DKOILO_ENABLE_AI="${AI}" \
    -DKOILO_ENABLE_AUDIO="${AUDIO}" \
    -DKOILO_ENABLE_PARTICLES="${PARTICLES}" \
    "${extra_args[@]+"${extra_args[@]}"}"
}

cmake_build() {
  if [ "$GENERATOR" = "Ninja" ]; then
    cmake --build "${BUILD_DIR}" --config "${CONFIG}" --target "$@"
  else
    cmake --build "${BUILD_DIR}" --config "${CONFIG}" --parallel "${NUM_JOBS}" --target "$@"
  fi
}

cmake_build_all() {
  if [ "$GENERATOR" = "Ninja" ]; then
    cmake --build "${BUILD_DIR}" --config "${CONFIG}"
  else
    cmake --build "${BUILD_DIR}" --config "${CONFIG}" --parallel "${NUM_JOBS}"
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
    cmake_configure -DKL_BUILD_TESTS=ON
    cmake_build koilo_tests koilo_script_language_tests
    ctest --test-dir "${BUILD_DIR}" --output-on-failure ;;
  headercheck)
    check_namespace
    cmake_configure ; cmake_build koilo_headercheck ;;
  ctest)
    ctest --test-dir "${BUILD_DIR}" --output-on-failure ;;
  all)
    check_namespace
    cmake_configure
    cmake_build_all ;;
  clean)
    echo -e "${YELLOW}Removing ${BUILD_DIR}${NC}"
    rm -rf "${BUILD_DIR}" ;;
  help)
    usage ;;
  *)
    echo -e "${RED}Unknown command: ${cmd}${NC}"
    usage
    exit 1 ;;
esac
