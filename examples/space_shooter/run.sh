#!/bin/bash
# Run the Space Shooter example via the generic koilo runner.
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
exec "${REPO_ROOT}/build/koilo" --vulkan --script "${SCRIPT_DIR}/space_shooter.ks" \
     --title "KoiloEngine - Space Shooter" --software --vsync "$@"
