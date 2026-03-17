#!/bin/bash
# Run the UI Showcase example via the generic koilo runner.
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
exec "${REPO_ROOT}/build/koilo" --vulkan --script "${SCRIPT_DIR}/ui_showcase.ks" \
     --title "KoiloEngine - UI Showcase" "$@"
