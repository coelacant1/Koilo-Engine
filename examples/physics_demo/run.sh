#!/bin/bash
# Run the Physics + Aerodynamics example via the generic koilo runner.
# Default: Vulkan. Pass --opengl or --software to switch backend.
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
exec "${REPO_ROOT}/build/koilo" --script "${SCRIPT_DIR}/physics_demo.ks" \
     --title "KoiloEngine - Physics + Aero" "$@"
