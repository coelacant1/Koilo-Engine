# KoiloScript Language Support for VSCode

Syntax highlighting, bracket matching, and comment toggling for `.ks` files.

## Install

```bash
cd tools/vscode-koiloscript
./install.sh
```

Or manually: `code --install-extension koiloscript-1.0.0.vsix`

## Highlighted Elements

| Category | Examples |
|----------|---------|
| Control | `if`, `else`, `elseif`, `while`, `for`, `in`, `return`, `break`, `continue`, `yield` |
| Storage | `var`, `fn`, `function`, `class`, `new`, `auto`, `import` |
| Logic | `and`, `or`, `not` |
| Signals | `signal`, `emit`, `set_state` |
| Sections | `DISPLAY`, `SCENE`, `ASSETS`, `STATES`, `UPDATE`, `CAMERA`, `MATERIAL`, etc. |
| Builtins | `print`, `sin`, `cos`, `lerp`, `clamp`, `random`, `connect`, `start_coroutine`, etc. |
| Globals | `self`, `display`, `scene`, `physics`, `input`, `ui`, `audio`, `camera`, `debug` |
| Constants | `true`, `false`, `null` |
| Operators | `==`, `!=`, `->`, `?` (ternary), `+`, `-`, `*`, `/`, `%` |
