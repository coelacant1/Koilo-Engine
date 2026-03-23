#!/bin/bash
# Run the Syntax Demo example via the generic koilo runner.
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
exec "${REPO_ROOT}/build/koilo" --script "${SCRIPT_DIR}/syntax_demo.ks" \
     --title "KoiloEngine - Syntax Demo" "$@"
