#!/bin/bash
# Run the 3D Scene example via the generic koilo runner.
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
exec "${REPO_ROOT}/build/koilo" --vulkan --script "${SCRIPT_DIR}/scene_3d.ks" \
     --title "KoiloEngine - 3D Scene" "$@"
