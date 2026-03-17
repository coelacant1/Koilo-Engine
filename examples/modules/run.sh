#!/bin/bash
# Run the Modules Demo example via the generic koilo runner.
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
exec "${REPO_ROOT}/build/koilo" --script "${SCRIPT_DIR}/demo_all_modules.ks" \
     --title "KoiloEngine - Modules" --modules-dir "${REPO_ROOT}/build/modules" "$@"
