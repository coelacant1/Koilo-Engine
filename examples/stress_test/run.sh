#!/bin/bash
# Run the Stress Test example via the generic koilo runner.
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
exec "${REPO_ROOT}/build/koilo" --vulkan --script "${SCRIPT_DIR}/stress_test.ks" \
     --title "KoiloEngine - Stress Test" "$@"
