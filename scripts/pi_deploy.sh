#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# ---------------------------------------------------------------
# pi_deploy.sh - Cross-compile, deploy, and run Koilo on the Pi
#
# Usage:
#   ./scripts/pi_deploy.sh <script.ks> [options] [--set key value ...]
#
# Examples:
#   ./scripts/pi_deploy.sh examples/led_demo/led_demo.ks --led --preview --uncapped
#   ./scripts/pi_deploy.sh examples/led_demo/led_demo.ks --led
#   ./scripts/pi_deploy.sh examples/obj_scene/obj_scene.ks
#   ./scripts/pi_deploy.sh --build-only
#
# ---------------------------------------------------------------
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build_pi"
TOOLCHAIN="${ROOT_DIR}/cmake/toolchain-pi-aarch64.cmake"

# -- Pi connection (override with env vars) --
PI_HOST="${PI_HOST:-192.168.3.161}"
PI_USER="${PI_USER:-coela}"
PI_KEY="${PI_KEY:-$HOME/.ssh/pi_koilo}"
PI_DIR="${PI_DIR:-~/koilo}"

SSH_OPTS="-i ${PI_KEY} -o StrictHostKeyChecking=no -o ConnectTimeout=5"
SSH_CMD="ssh ${SSH_OPTS} ${PI_USER}@${PI_HOST}"

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
CYAN='\033[0;36m'
NC='\033[0m'

REMOTE_PID=""
TAIL_PID=""
CLEANED_UP=false

cleanup() {
    if ${CLEANED_UP}; then return; fi
    CLEANED_UP=true
    echo ""
    if [ -n "${TAIL_PID}" ]; then
        kill "${TAIL_PID}" 2>/dev/null || true
        wait "${TAIL_PID}" 2>/dev/null || true
    fi
    if [ -n "${REMOTE_PID}" ]; then
        echo -e "${YELLOW}Stopping remote engine (PID ${REMOTE_PID})...${NC}"
        ${SSH_CMD} "kill ${REMOTE_PID} 2>/dev/null; sleep 0.5; kill -9 ${REMOTE_PID} 2>/dev/null" 2>/dev/null || true
        echo -e "${GREEN}Remote process stopped.${NC}"
    fi
}
trap cleanup EXIT INT TERM

usage() {
    cat <<EOF
Koilo Pi Deploy - Cross-compile, deploy, and run on Raspberry Pi

Usage: $(basename "$0") [options] [script.ks] [--set key value ...]

Options:
  -b, --build-only     Build and deploy, don't run
  -r, --run-only       Skip build, deploy and run
  -n, --no-build       Skip build, deploy existing binary
  -c, --configure      Force CMake reconfigure
  -j N                 Parallel build jobs (default: 8)
  --led                Enable LED volume (USB transport, HUB75 defaults)
  --preview            Enable live preview server
  --uncapped           Disable frame rate cap
  -h, --help           Show this help

Environment:
  PI_HOST    Pi IP address       (default: 192.168.3.161)
  PI_USER    Pi username          (default: coela)
  PI_KEY     SSH key path         (default: ~/.ssh/pi_koilo)
  PI_DIR     Remote directory     (default: ~/koilo)

Examples:
  $(basename "$0") examples/led_demo/led_demo.ks --led --preview
  $(basename "$0") examples/led_demo/led_demo.ks --led --set led.brightness 128
  $(basename "$0") --build-only
  PI_HOST=10.0.0.5 $(basename "$0") examples/obj_scene/obj_scene.ks
EOF
}

# -- Parse arguments --
DO_BUILD=true
DO_DEPLOY=true
DO_RUN=true
DO_CONFIGURE=false
BUILD_JOBS="8"
SCRIPT_FILE=""
ENGINE_ARGS=()
PREVIEW_ENABLED=false
LED_ENABLED=false
UNCAPPED=false

while [ $# -gt 0 ]; do
    case "$1" in
        -b|--build-only)   DO_RUN=false ;;
        -r|--run-only)     DO_BUILD=false ;;
        -n|--no-build)     DO_BUILD=false ;;
        -c|--configure)    DO_CONFIGURE=true ;;
        -j)                shift; BUILD_JOBS="$1" ;;
        --led)             LED_ENABLED=true ;;
        --preview)         PREVIEW_ENABLED=true ;;
        --uncapped)        UNCAPPED=true ;;
        -h|--help)         usage; exit 0 ;;
        --set)             ENGINE_ARGS+=("--set" "$2" "$3"); shift 2 ;;
        -*)                ENGINE_ARGS+=("$1") ;;
        *)
            if [ -z "${SCRIPT_FILE}" ] && [[ "$1" == *.ks ]]; then
                SCRIPT_FILE="$1"
            else
                ENGINE_ARGS+=("$1")
            fi
            ;;
    esac
    shift
done

# -- Apply presets --
if ${LED_ENABLED}; then
    ENGINE_ARGS+=(
        "--set" "led.enabled"        "1"
        "--set" "led.transport_type"  "usb"
        "--set" "led.usb_device"     "/dev/ttyACM1"
        "--set" "led.camera_layout"  "assets/cameras/HUB75_64x64.klcam"
        "--set" "led.count"          "4096"
        "--set" "led.brightness"     "255"
        "--set" "led.gamma"          "1.0"
        "--set" "led.filter"         "nearest"
        "--set" "led.flip_x"         "1"
    )
fi

if ${PREVIEW_ENABLED}; then
    ENGINE_ARGS+=("--set" "preview.port" "8080" "--set" "preview.fps" "15")
fi

if ${UNCAPPED}; then
    ENGINE_ARGS+=("--uncapped")
fi

# -- Step 1: Cross-compile --
if ${DO_BUILD}; then
    echo -e "${CYAN}=== Cross-compiling for aarch64 ===${NC}"

    if ${DO_CONFIGURE} || [ ! -f "${BUILD_DIR}/build.ninja" ]; then
        echo -e "${GREEN}Configuring...${NC}"
        cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -G Ninja \
            -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN}" \
            -DCMAKE_BUILD_TYPE=RelWithDebInfo \
            -DKOILO_USE_VULKAN=ON \
            -DKOILO_USE_OPENGL=OFF \
            -DKOILO_ENABLE_AUDIO=OFF \
            -DKOILO_ENABLE_LED_VOLUME=ON \
            -DKOILO_ENABLE_LIVE_PREVIEW=ON
    fi

    ninja -C "${BUILD_DIR}" -j"${BUILD_JOBS}" koilo
    echo -e "${GREEN}Build complete.${NC}"
fi

# -- Step 2: Deploy --
if ${DO_DEPLOY}; then
    echo -e "${CYAN}=== Deploying to ${PI_USER}@${PI_HOST}:${PI_DIR} ===${NC}"

    ${SSH_CMD} "mkdir -p ${PI_DIR}"

    rsync -az -e "ssh ${SSH_OPTS}" \
        "${BUILD_DIR}/koilo" "${PI_USER}@${PI_HOST}:${PI_DIR}/koilo"

    # Deploy compiled shaders (SPIR-V + KSO)
    if [ -d "${BUILD_DIR}/shaders" ]; then
        ${SSH_CMD} "mkdir -p ${PI_DIR}/build/shaders"
        rsync -az --delete -e "ssh ${SSH_OPTS}" \
            "${BUILD_DIR}/shaders/" "${PI_USER}@${PI_HOST}:${PI_DIR}/build/shaders/"
    fi

    rsync -az --delete -e "ssh ${SSH_OPTS}" \
        "${ROOT_DIR}/assets/" "${PI_USER}@${PI_HOST}:${PI_DIR}/assets/"
    rsync -az --delete -e "ssh ${SSH_OPTS}" \
        "${ROOT_DIR}/examples/" "${PI_USER}@${PI_HOST}:${PI_DIR}/examples/"

    echo -e "${GREEN}Deploy complete.${NC}"
fi

# -- Step 3: Run --
if ${DO_RUN}; then
    if [ -z "${SCRIPT_FILE}" ]; then
        echo -e "${RED}No script file specified. Use: $(basename "$0") <script.ks>${NC}"
        exit 1
    fi

    REMOTE_CMD="cd ${PI_DIR} && SDL_VIDEODRIVER=offscreen ./koilo --script ${SCRIPT_FILE}"
    for arg in "${ENGINE_ARGS[@]+"${ENGINE_ARGS[@]}"}"; do
        REMOTE_CMD+=" ${arg}"
    done

    echo -e "${CYAN}=== Running on Pi ===${NC}"
    echo -e "${GREEN}> ${REMOTE_CMD}${NC}"

    if ${PREVIEW_ENABLED}; then
        echo -e "${YELLOW}Live preview: http://${PI_HOST}:8080/${NC}"
    fi
    echo -e "${YELLOW}Press Ctrl+C to stop.${NC}"
    echo ""

    REMOTE_PID=$(${SSH_CMD} "nohup bash -c '${REMOTE_CMD}' > /tmp/koilo_stdout.log 2>&1 & echo \$!")
    echo -e "${GREEN}Remote PID: ${REMOTE_PID}${NC}"

    ${SSH_CMD} "tail -f /tmp/koilo_stdout.log" &
    TAIL_PID=$!
    wait ${TAIL_PID} 2>/dev/null || true
fi
