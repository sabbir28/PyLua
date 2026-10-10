# PyLua

<p align="center">
  <strong>A modern, high-performance programming language combining Lua's lightweight speed, Python's clean imports, C-style curly braces, and an integrated hardware-accelerated 2D Game Engine.</strong>
</p>

<p align="center">
  <a href="https://github.com/sabbir28/PyLua/actions/workflows/release.yml"><img src="https://img.shields.io/github/actions/workflow/status/sabbir28/PyLua/release.yml?branch=master&style=flat-square&label=Build%20%26%20Release" alt="CI Status"></a>
  <a href="https://github.com/sabbir28/PyLua/releases"><img src="https://img.shields.io/github/v/release/sabbir28/PyLua?style=flat-square&color=blue" alt="Latest Release"></a>
  <img src="https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20Android-green?style=flat-square" alt="Platforms">
  <img src="https://img.shields.io/badge/Base-Lua%205.5.1%20LEngine-purple?style=flat-square" alt="Lua Version">
  <img src="https://img.shields.io/badge/License-MIT-orange?style=flat-square" alt="License">
</p>

---

## 🌟 Why PyLua?

Traditional Lua is famously fast and compact, but often frustrates developers with verbose block syntax (`then ... end`, `do ... end`), repetitive `local x = require("x")` imports, accidental global variable leaks when omitting `local`, and a minimal standard library lacking vectors, HTTP, or a native graphics engine.

**PyLua solves this by modernizing the language from the ground up:**

* 🐍 **Python-Style Imports**: Use `import engine`, `import os`, and `import vector` directly.
* 🛡️ **Auto-Local Variable Safety**: Variables declared without `local` automatically default to local scope, eliminating global state pollution bugs.
* 🔣 **C-Style Curly Braces**: Write `if (condition) { ... }`, `fn = function() { ... }`, and loops with familiar `{}` syntax.
* ⚙️ **Modern Operators**: Native `!=` inequality, `//` floor division, and bitwise operators (`&`, `|`, `~`, `<<`, `>>`).
* 🔄 **100% Backward Compatible**: All standard Lua 5.4 syntax, modules, and idioms continue to work without modification.
* 🎮 **Built-in 2D Game Engine**: Hardware-accelerated OpenGL graphics, rigid-body physics, miniaudio sound, sprite sheet animators, multi-monitor windowing, and multithreaded background workers right out of the box.
* 📦 **Batteries Included**: Preloaded extensions for 2D/3D vectors (`vector`), HTTP networking (`request`), native Windows UI (`gui`), and cryptography (`hash`).

---

## 🚀 Quick Comparison

| Feature | Standard Lua 5.4 | PyLua |
| :--- | :--- | :--- |
| **Module Imports** | `local os = require("os")` | `import os` |
| **Block Delimiters** | `if cond then ... end` | `if (cond) { ... }` *(or traditional `then ... end`)* |
| **Function Bodies** | `function() ... end` | `function() { ... }` *(or traditional `end`)* |
| **Undeclared Variables** | Leaks to Global `_G` (Bug-prone) | **Auto-Local** (Safe by default) |
| **Not-Equal Operator** | `~=` | `!=` and `~=` |
| **Vector Math** | None (Third-party library) | Built-in `vector.Vector2` & `Vector3` |
| **HTTP Requests** | None (Requires LuaSocket/cURL) | Built-in `request.get()` / `request.post()` |
| **Game Engine** | None | Built-in `engine.*` (Window, Physics, Graphics, Audio, UI) |

---

## 📦 Installation

### Windows Installer (Recommended)
Download the latest `PyLua-<version>-Setup.exe` from the [Releases](https://github.com/sabbir28/PyLua/releases) page. The installer automatically registers `pylua.exe` in your system `PATH` and bundles all runtime dependencies (GLFW, OpenGL).

### Standalone Executable
You can also download the precompiled standalone binaries for:
* **Windows x64**: `pylua.exe` + `glfw3.dll`
* **Linux x64**: `pylua.tar.gz`
* **Android**: `libpylua_static.a` for `arm64-v8a`, `armeabi-v7a`, `x86`, `x86_64`

### Building from Source

PyLua uses modern CMake (3.20+) and builds with any standard C99 compiler (MSVC on Windows, GCC or Clang on Linux):

```bash
# 1. Clone repository
git clone https://github.com/sabbir28/PyLua.git
cd PyLua

# 2. Configure with CMake
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# 3. Compile
cmake --build build --parallel

# 4. Verify build
./build/bin/pylua -v
# Output: Lua 5.5.1  The LEngine
```

---

## 💻 Language Syntax Tour

### 1. Python-Style `import`
```lua
import os
import engine
import vector

print("Operating System: " .. (os.getenv("OS") or "Unknown"))
```

### 2. C-Style Braces & Conditionals
```lua
score = 85

if (score >= 90) {
    print("Grade: A")
} elseif (score >= 80) {
    print("Grade: B")
} else {
    print("Keep studying!")
}
```

### 3. Auto-Local Variable Scoping
```lua
function calculateDistance(x1, y1, x2, y2) {
    -- dx and dy are automatically local to this function!
    dx = x2 - x1
    dy = y2 - y1
    return math.sqrt(dx * dx + dy * dy)
}

calculateDistance(0, 0, 3, 4)
print(dx) -- Output: nil (no accidental global variable leak!)
```

### 4. Braced Functions & Loops
```lua
-- Braced Anonymous Function
onHit = function(damage) {
    playerHealth = playerHealth - damage
    return playerHealth > 0
}

-- Braced For-Loop
for i, item in ipairs(inventory) {
    print("Slot " .. i .. ": " .. item.name)
}
```

### 5. Modern Inequality & Bitwise Operators
```lua
if (player.state != "dead") {
    flags = mask & 0xFF
    bitVal = 1 << 3
}
```

---

## 🎮 The PyLua 2D Game Engine

PyLua includes a lightweight, hardware-accelerated 2D game framework inspired by Unity and LÖVE2D.

### Minimal Game Loop Example
```lua
import engine

-- 1. Initialize Window
win = engine.Window.create("PyLua Game", 1280, 720)

-- 2. Setup Physics World
world = engine.Physics.createWorld(0, -980)
player = world:addBody(1, 640, 360) -- Dynamic body
player:setCircle(20)

-- 3. Game Lifecycle
engine.run({
    window = win,
    physicsWorld = world,
    
    init = function() {
        print("Game Started!")
    },
    
    update = function(dt) {
        -- Continuous Keyboard Input
        if (engine.Input.isKeyDown("space")) {
            player:setVelocity(0, 400)
        }
        
        -- Single-Frame Latch Input
        if (engine.Input.isKeyJustPressed("escape")) {
            engine.stop()
        }
    },
    
    draw = function() {
        engine.Graphics.clear(0.08, 0.08, 0.12, 1.0)
        
        -- Draw Player Circle
        x, y = player:getPosition()
        engine.Graphics.drawRect(x - 20, y - 20, 40, 40, 0.2, 0.8, 0.4, 1.0)
        
        -- Immediate-Mode UI
        if (engine.UI.button("QUIT", 20, 20, 100, 35)) {
            engine.stop()
        }
    }
})
```

### Engine Subsystems Overview

* **`engine.Window`**: Window management, multi-monitor detection (`getMonitors()`), resolution sizing, and fullscreen toggling.
* **`engine.Graphics`**: 2D immediate-mode rendering (`drawRect`, `drawCircle`, `drawLine`, `drawSprite`, `drawText`).
* **`engine.Physics`**: Rigid-body physics simulation, dynamic/static bodies, circle/box colliders, velocities, and gravity.
* **`engine.Audio`**: Low-latency multi-channel audio powered by `miniaudio` (streaming, looping, volume, pitch).
* **`engine.Assets`**: Image asset loading (`loadTexture()`) with GPU texture binding.
* **`engine.Animation`**: Sprite sheet slicing (`newSpriteSheet`), timed keyframe animations, and `.panim` animator files.
* **`engine.UI`**: Immediate-mode UI elements (`panel`, `button`, `label`).
* **`engine.Scene`**: Entity-script ecosystem (`Scene.createEntity("Player")`, `entity:addScript(script)`).
* **`engine.Screen`**: Multi-screen scene manager (`Screen.set(GameScreen)`, `onEnter`, `update`, `draw`, `onExit`).
* **`engine.createWorker(script)`**: Parallel OS threads running separate Lua VMs for background tasks (music, pathfinding).

---

## 📚 Standard & Extended Libraries

PyLua includes standard Lua modules along with powerful built-in extensions:

### `vector` — 2D & 3D Vector Math
```lua
import vector

v1 = vector.Vector2(3, 4)
v2 = vector.Vector2(1, 2)
v3 = v1 + v2          -- Vector2(4, 6)
dist = v1:length()    -- 5.0
dot = v1:dot(v2)      -- 11.0

v3d = vector.Vector3(1, 0, 0):cross(vector.Vector3(0, 1, 0)) -- Vector3(0, 0, 1)
```

### `request` — HTTP Client
```lua
import request

res = request.get("https://api.github.com/zen")
print("Status: " .. res.status_code)
print("Content: " .. res.body)
```

### `gui` — Native Windows Desktop Controls
```lua
import gui

win = gui.Window("PyLua App", 100, 100, 350, 200)
btn = win:Button("Click Me", 20, 20, 120, 35)

btn:OnClick(function() {
    print("Button pressed!")
})

win:Show()
```

### `hash` — Cryptographic Hashing
```lua
import hash

hashVal = hash.md5("secret_string")
print("MD5: " .. hashVal)
```

---

## 🔍 Live Visual CPU & Bytecode Debugger

PyLua features a built-in interactive **x64dbg / OllyDbg-style visual CPU & bytecode debugger**:

```bash
pylua --debug my_script.lua
```

* **Interactive Disassembly**: View 32-bit bytecode instructions with hex addresses (`0000`), raw hex machine bytes (`43 00 80 00`), decoded opcodes, operands, and evaluated comments.
* **Visual Branch Arrows**: Colored directional branch lines connecting jump instructions (`OP_JMP`, `OP_FORLOOP`) directly to target destination offsets.
* **Execution Pointer**: Real-time `▶ PC` pointer highlighting the currently paused or executing instruction.
* **Virtual Registers Panel**: Live inspection of registers `R[0] .. R[N]` with dynamic type tags (`integer`, `float`, `string`, `table`, etc.).
* **Constants Pool & Upvalues**: Immediate visibility into string/numeric constant tables (`K0 .. KM`) and function closure upvalues.
* **Breakpoints**: Click the breakpoint column or press <kbd>F2</kbd> to set/clear instruction breakpoints.
* **Keyboard Hotkeys**: <kbd>F5</kbd> (Run), <kbd>F7</kbd> (Step Into single bytecode), <kbd>F8</kbd> (Step Over), <kbd>F2</kbd> (Toggle Breakpoint), <kbd>P</kbd> (Pause), <kbd>1-5</kbd> (Switch Tabs).

---

## 📖 Documentation Website

Comprehensive API documentation, interactive examples, and guides are available in the [`docs/`](docs/) directory:

* Open [`docs/index.html`](docs/index.html) in your browser for the full interactive website.
* Deployable instantly with **GitHub Pages** (Settings &rarr; Pages &rarr; Source `/docs`).

---

## 🤝 Contributing

Contributions, bug reports, and suggestions are welcome!
1. Fork the repository.
2. Create your feature branch (`git checkout -b feature/cool-feature`).
3. Commit your changes (`git commit -m "Add cool feature"`).
4. Push to the branch (`git push origin feature/cool-feature`).
5. Open a Pull Request.

---

## 📄 License

PyLua is released under the [MIT License](LICENSE). Lua core is Copyright &copy; 1994–2026 Lua.org, PUC-Rio.
