#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Install the KoiloScript VSCode extension
# Usage: ./install.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VSIX="$SCRIPT_DIR/koiloscript-1.0.0.vsix"

# Package the extension
echo "Packaging KoiloScript extension..."
cd "$SCRIPT_DIR"
npx @vscode/vsce package --no-dependencies -o "$VSIX" 2>/dev/null

# Install into VSCode
if command -v code &>/dev/null; then
    echo "Installing into VSCode..."
    code --install-extension "$VSIX" --force
    echo "Done. Reload VSCode to activate."
else
    echo "Built: $VSIX"
    echo "'code' CLI not found - install manually via Extensions > Install from VSIX."
fi
