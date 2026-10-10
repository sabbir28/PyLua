# PyLua Documentation Website

This directory contains the multi-page, comprehensive documentation suite and interactive website for the **PyLua Programming Language & 2D Game Engine**.

## 📖 Viewing the Documentation

### Option 1: Direct File Viewing (No Server Required)
Simply open `docs/index.html` in any web browser (Chrome, Firefox, Edge, Safari). All styles, navigation scripts, and search features are 100% self-contained and run locally offline with zero dependencies.

### Option 2: Local HTTP Server
You can also preview the documentation using any static web server:

```bash
# Python 3
cd docs
python -m http.server 8000

# Node.js npx
npx serve docs
```

Then visit [http://localhost:8000](http://localhost:8000) in your browser.

### Option 3: GitHub Pages Deployment
In your GitHub repository settings:
1. Navigate to **Settings** > **Pages**.
2. Under **Build and deployment**, select:
   * **Source**: `Deploy from a branch`
   * **Branch**: `master` (or `main`)
   * **Folder**: `/docs`
3. Click **Save**. Your documentation site will be automatically published on GitHub Pages.

---

## 📚 Multi-Page Documentation Directory

```
docs/
├── index.html                   # Master Documentation Portal & Live Tour
├── style.css                    # Dark/Light theme, responsive layout, opcode cards
├── app.js                       # Multi-page search engine ('/'), theme toggle, copy buttons
├── README.md                    # This file
│
├── syntax/                      # Deep-Dive Language Specifications
│   ├── index.html               # 1. Lexical Structure, Character Encoding & Streaming Lexer
│   ├── types.html               # 2. Type System, 16-Byte Tagged Values (TValue) & Metatables
│   ├── variables.html           # 3. Variables, Scoping, Auto-Local Engine & Upvalues
│   ├── control-flow.html        # 4. Control Flow: if/else with {}, loops, and jump opcodes
│   ├── operators.html           # 5. Operators: != inequality, bitwise, precedence table
│   ├── functions.html           # 6. Functions, Closures, Varargs, Method Sugar & Tail Calls
│   └── modules.html             # 7. Module System: Python-style import & require pipeline
│
├── runtime/                     # Compiler & Virtual Machine Architecture
│   ├── index.html               # 1. Compiler Pipeline: Lexer -> Parser -> CodeGen -> Proto
│   ├── bytecode-vm.html         # 2. Register-Based VM, Sliding Windows & Dispatch Loop
│   ├── opcodes.html             # 3. Complete 32-Bit Bytecode Opcode Reference
│   └── gc-memory.html           # 4. Memory Management & Generational Garbage Collector
│
├── standard-library/            # Complete Standard Library Reference
│   ├── index.html               # Standard Library Overview & Globals
│   ├── base.html                # Base Built-ins: print, assert, pcall, xpcall, type, tostring
│   ├── vector.html              # Vector Math: 2D & 3D Vectors, Overloaded Operators
│   ├── string-utf8.html         # String Formatting, Pattern Matching & UTF-8 Codepoints
│   ├── table.html               # Table Manipulation: Sorting, Slices, Pack/Unpack
│   ├── math.html                # Math Library: Trigonometry, Constants, xoshiro256** PRNG
│   ├── io-os.html               # File I/O Streams, RAII <close>, and OS Subsystems
│   ├── request.html             # HTTP/HTTPS Networking Client (GET, POST, JSON)
│   ├── gui.html                 # Native Desktop GUI Windows, Controls & Dialogs
│   └── hash.html                # Cryptographic Digests: MD5 and SHA-256
│
└── engine/                      # 2D Game Engine (LEngine) Reference
    ├── index.html               # Game Engine Architecture & The Unity-Like Loop
    ├── window.html              # Window Subsystem, Multi-Monitor Displays & Fullscreen
    ├── graphics.html            # OpenGL 2D Immediate-Mode Rendering & Sprites
    ├── physics.html             # Rigid-Body Physics: Worlds, Dynamic/Static Bodies, Gravity
    ├── audio.html               # Miniaudio Sound System & 3D Spatial Audio
    ├── input.html               # Real-Time Keyboard Polling & Mouse Coordinates
    ├── ui.html                  # Immediate-Mode In-Game UI: Buttons, Sliders, Panels
    ├── animation.html           # Sprite Sheet Animator & Visual Editor PRO
    ├── entity.html              # Entity-Script Component Model & Lifecycle Dispatch
    └── workers.html             # Multithreaded Background Workers & Concurrency
```
