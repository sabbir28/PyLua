# PyLua Documentation Website

This directory contains the full documentation website for the **PyLua Programming Language & 2D Game Engine**.

## 📖 Viewing the Documentation

### Option 1: Direct File Viewing (No Server Required)
Simply double-click or open `docs/index.html` in any modern web browser (Chrome, Firefox, Edge, Safari). Everything is self-contained with zero external dependencies.

### Option 2: Local HTTP Server
You can serve the documentation using any local static file server:

```bash
# Python 3
cd docs
python -m http.server 8000

# Node.js npx
npx serve docs
```

Then visit [http://localhost:8000](http://localhost:8000) in your browser.

### Option 3: GitHub Pages
In your GitHub repository settings:
1. Navigate to **Settings** > **Pages**.
2. Under **Build and deployment**, select:
   * **Source**: `Deploy from a branch`
   * **Branch**: `master` (or `main`)
   * **Folder**: `/docs`
3. Click **Save**. Your documentation site will be live on GitHub Pages automatically!

---

## 📚 What's Covered in the Documentation
1. **Introduction & Quickstart**: Installing PyLua on Windows, Linux, and Android, command line usage, and your first program.
2. **PyLua Syntax & Modern Extensions**:
   * `import <module>` (Python-style module imports)
   * C-style curly braces for `if (cond) { ... }`, loops, and functions
   * Auto-local variable scoping (no accidental global leaks)
   * Modern operators (`!=`, `//`, `&`, `|`, `~`, `<<`, `>>`)
   * 100% backward compatibility with Lua 5.4 syntax
3. **Idiomatic Coding Style Guide**: Best practices for naming conventions, code layout, brace styles, and ecosystem architecture.
4. **Extended Standard Libraries**:
   * `vector` (Vector2 and Vector3 with vector math and operator overloads)
   * `hash` (MD5 hashing)
   * `gui` (Native Windows GUI dialogs, controls, and event listeners)
   * `request` (HTTP client requests)
5. **PyLua 2D Game Engine**:
   * Game loop lifecycle (`engine.run`, `engine.stop`, `engine.getDeltaTime`)
   * `engine.Window` (Multi-monitor detection, fullscreen, custom resolution)
   * `engine.Graphics` (Colored primitives, sprites, and bitmap text)
   * `engine.Physics` (Physics world, dynamic & static rigid bodies, gravity, collision detection)
   * `engine.Input` (Keyboard, mouse, single-frame latched key events)
   * `engine.Audio` (Sound effects, looping music channels, volume/pitch control)
   * `engine.Assets` (Texture loader with hardware acceleration)
   * `engine.Animation` (Sprite sheets, keyframe animations, Animator multi-sequence state machine)
   * `engine.UI` (Immediate-mode buttons, panels, labels)
   * `engine.Scene` (Entity system with multi-script component attachment)
   * `engine.Screen` (Screen transition state machine with lifecycle callbacks)
   * `engine.createWorker` (Multithreaded background workers)
6. **Full Game Examples**:
   * Complete Snake game using the Screen and immediate UI system
   * Parallel background audio worker

