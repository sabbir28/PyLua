# PyLua VS Code Extension

This extension provides language support for PyLua, a custom version of Lua featuring:
- **`class` keyword**: Support for Object-Oriented Programming with brace-style blocks.
- **`emu` keyword**: Automatic global table registration for enums.
- **`request` library**: Native HTTP request support via WinHTTP.
- **Brace support**: Use `{}` for classes and optionally in function bodies.

## Installation

1. Copy the `ide/vscode` folder to your VS Code extensions directory:
   - Windows: `%USERPROFILE%\.vscode\extensions\pylua-support`
2. Restart VS Code.
3. Open a `.lua` or `.pylua` file.

## Features

- **Syntax Highlighting**: Supports `class`, `emu`, and brace blocks.
- **Code Completion**: Basic completion for `class`, `emu`, and `request`.
- **Indentation**: Automatic indentation for brace-style and traditional Lua blocks.

## Language Server (Advanced)

The `server.js` provides a scaffold for a Language Server Protocol (LSP). To use it fully:
1. Run `npm install` in the extension directory.
2. Update the `extension.js` (not included) to launch the server.
