#!/bin/bash
# Run the OBJ Scene example via the generic koilo runner.
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
exec "${REPO_ROOT}/build/koilo" --vulkan --script "${SCRIPT_DIR}/obj_scene.ks" \
     --title "KoiloEngine - OBJ Scene" "$@"
