# PyLua Engine API Reference

The **PyLua 2D Game Engine** is an integrated, hardware-accelerated 2D game framework built directly into PyLua. It features a Unity-inspired game loop, an entity-script component architecture, rigid-body physics, immediate-mode rendering and UI, low-latency audio, sprite animators, and multithreaded worker threads.

---

## 🚀 Getting Started

Import the engine module at the top of your script:

```lua
import engine
```

---

## 🏛️ Core Engine (`engine`)

### Lifecycle & Loop: `engine.run(config)`

Starts the main game loop. Handles GLFW window events, auto-steps physics simulation, triggers entity script updates, and swaps frame buffers.

```lua
engine.run({
    window = win,           -- Required: Window instance from engine.Window.create
    physicsWorld = world,   -- Optional: Auto-steps physics world each frame
    init = function(),      -- Optional: Called once at engine startup
    update = function(dt),  -- Optional: Called every frame with delta time in seconds
    draw = function()       -- Optional: Called every frame for rendering
})
```

### Global Control Functions

| Function | Signature | Description |
| :--- | :--- | :--- |
| `engine.stop()` | `()` &rarr; `void` | Signals the engine loop to terminate cleanly on the next frame. |
| `engine.getDeltaTime()` | `()` &rarr; `number` | Returns elapsed time in seconds between the current and previous frame. |
| `engine.createWorker(path)` | `(string)` &rarr; `Worker` | Launches a background OS thread running a separate Lua VM for parallel tasks. |
| `engine.destroyWorker(worker)`| `(Worker)` &rarr; `void` | Cleans up and terminates a background worker thread. |

---

## 🖼️ Window Subsystem (`engine.Window`)

Handles window management, monitor querying, resolution changes, and fullscreen modes.

### Functions

| Function | Signature | Description |
| :--- | :--- | :--- |
| `create(title, w, h, [fs, monitor])` | `(string, int, int, [bool, int])` &rarr; `Window` | Creates a new GLFW OpenGL window. `fs` enables fullscreen on `monitor` index. |
| `getMonitors()` | `()` &rarr; `table` | Returns array of detected displays: `{ {name, width, height, x, y}, ... }`. |

### `Window` Methods

* `win:isOpen()` &rarr; `bool`: Returns `true` while the window remains open.
* `win:close()` &rarr; `void`: Signals window destruction.
* `win:pollEvents()` &rarr; `void`: Polls operating system window events.
* `win:getSize()` &rarr; `int, int`: Returns current `width, height`.
* `win:setSize(w, h)` &rarr; `void`: Resizes window client area.
* `win:setTitle(title)` &rarr; `void`: Updates title bar text.

---

## 🎨 Graphics Subsystem (`engine.Graphics`)

Provides hardware-accelerated 2D immediate-mode rendering for shapes, sprites, and bitmap text.

| Function | Signature | Description |
| :--- | :--- | :--- |
| `clear(r, g, b, a)` | `(num, num, num, num)` &rarr; `void` | Clears color buffer with normalized RGBA values (0.0 to 1.0). |
| `drawRect(x, y, w, h, r, g, b, a)` | `(num, num, num, num, num, num, num, num)` &rarr; `void` | Renders a solid colored rectangle. |
| `drawCircle(x, y, radius, r, g, b, a)` | `(num, num, num, num, num, num, num)` &rarr; `void` | Renders a colored circle. |
| `drawLine(x1, y1, x2, y2, r, g, b, a)` | `(num, num, num, num, num, num, num, num)` &rarr; `void` | Renders a line between two points. |
| `drawSprite(texture, x, y, [w, h])` | `(Texture, num, num, [num, num])` &rarr; `void` | Renders a textured sprite. |
| `drawText(text, x, y, [scale, r, g, b, a])` | `(string, num, num, [num, num, num, num, num])` &rarr; `void` | Draws 2D bitmap text with customizable scale and color. |

---

## ⚖️ Physics Subsystem (`engine.Physics`)

A 2D rigid-body physics simulation engine supporting gravity, velocity, static colliders, and dynamic bodies.

### Functions

| Function | Signature | Description |
| :--- | :--- | :--- |
| `createWorld(gx, gy)` | `(number, number)` &rarr; `World` | Initializes a physics simulation world with gravity vector `(gx, gy)`. |

### `World` Methods

* `world:addBody(type, x, y)` &rarr; `Body`: Spawns a body. `type = 1` for dynamic bodies, `type = 0` for static bodies.
* `world:step(dt)` &rarr; `void`: Advances the physics simulation by delta time `dt`.

### `Body` Methods

* `body:setRect(w, h)` &rarr; `void`: Assigns an axis-aligned bounding box collider.
* `body:setCircle(radius)` &rarr; `void`: Assigns a circular collider.
* `body:setVelocity(vx, vy)` &rarr; `void`: Sets linear velocity.
* `body:getVelocity()` &rarr; `number, number`: Returns `vx, vy`.
* `body:setPosition(x, y)` &rarr; `void`: Moves body directly to `(x, y)`.
* `body:getPosition()` &rarr; `number, number`: Returns current `x, y` world position.

---

## ⌨️ Input Subsystem (`engine.Input`)

Single-frame latched keyboard and mouse input inspection.

| Function | Signature | Description |
| :--- | :--- | :--- |
| `isKeyDown(key)` | `(string/int)` &rarr; `bool` | Returns `true` if key is currently held down. Supports strings like `"space"`, `"w"`, `"enter"`. |
| `isKeyJustPressed(key)` | `(string/int)` &rarr; `bool` | Returns `true` strictly on the first frame a key is pressed down. |
| `isKeyJustReleased(key)` | `(string/int)` &rarr; `bool` | Returns `true` strictly on the frame a key is released. |
| `getMousePosition()` | `()` &rarr; `number, number` | Returns cursor coordinates `x, y`. |
| `isMouseButtonDown(btn)` | `(int)` &rarr; `bool` | Checks mouse button state (0 = Left, 1 = Right, 2 = Middle). |

---

## 🔊 Audio Subsystem (`engine.Audio`)

Low-latency audio playback engine backed by `miniaudio`. Supports WAV, MP3, FLAC, and OGG formats.

| Function | Signature | Description |
| :--- | :--- | :--- |
| `play(path, [vol, loop, pitch])` | `(string, [num, bool, num])` &rarr; `int` | Plays sound file. Returns active channel ID. |
| `stop(channelId)` | `([int])` &rarr; `void` | Stops playback on channel ID (or all channels if omitted). |
| `pause(channelId)` | `(int)` &rarr; `void` | Pauses playback on channel ID. |
| `resume(channelId)` | `(int)` &rarr; `void` | Resumes playback on channel ID. |
| `setVolume(channelId, vol)` | `(int, number)` &rarr; `void` | Sets channel volume (0.0 to 1.0). |
| `setMasterVolume(vol)` | `(number)` &rarr; `void` | Sets global master volume. |

---

## 🖼️ Assets Subsystem (`engine.Assets`)

Loads image textures into GPU memory with automatic dimension caching.

| Function | Signature | Description |
| :--- | :--- | :--- |
| `loadTexture(path)` | `(string)` &rarr; `Texture` | Loads PNG/JPEG image file into an OpenGL texture object. |

### `Texture` Methods

* `tex:getSize()` &rarr; `int, int`: Returns image pixel `width, height`.

---

## 🎞️ Animation Subsystem (`engine.Animation`)

Sprite sheet slicing, timed keyframes, and multi-sequence state machines.

| Function | Signature | Description |
| :--- | :--- | :--- |
| `newSpriteSheet(texture, frameW, frameH)` | `(Texture, int, int)` &rarr; `SpriteSheet` | Slices a texture into equal grid cells of width `frameW` and height `frameH`. |
| `new(sheet, keyframes, [loop])` | `(SpriteSheet, table, [bool])` &rarr; `Animation` | Creates animation sequence from keyframe table `{{frame=0, duration=0.1}, ...}`. |
| `load(path, [loop])` | `(string, [bool])` &rarr; `Animation` | Loads animation file directly. |
| `loadAnimator(path)` | `(string)` &rarr; `Animator` | Loads multi-sequence `.panim` animator file created in Animator Editor PRO. |

### Animation Methods

* `anim:draw(x, y, time, [w, h])` &rarr; `void`: Renders the active frame at world position `(x, y)`.

### Animator Methods

* `animator:play(sequenceName)` &rarr; `void`: Switches active state sequence (e.g. `"idle"`, `"run"`).
* `animator:draw(x, y, time, [w, h])` &rarr; `void`: Renders the active animation sequence.

---

## 🖥️ Immediate-Mode UI (`engine.UI`)

Lightweight UI elements rendered directly during the `draw()` pass.

| Function | Signature | Description |
| :--- | :--- | :--- |
| `button(text, x, y, w, h)` | `(string, num, num, num, num)` &rarr; `bool` | Renders a clickable button. Returns `true` if clicked this frame. |
| `panel(x, y, w, h, [r, g, b, a])` | `(num, num, num, num, [num, num, num, num])` &rarr; `void` | Renders a colored background container panel. |
| `label(text, x, y, [scale, r, g, b, a])` | `(string, num, num, [num, num, num, num, num])` &rarr; `void` | Draws UI text. |

---

## 📂 Scene & Entity System (`engine.Scene`)

An ecosystem where game entities live and receive lifecycle updates.

### Functions

| Function | Signature | Description |
| :--- | :--- | :--- |
| `Scene.createEntity(name)` | `(string)` &rarr; `Entity` | Instantiates a new active game entity in the scene. |

### `Entity` Methods

* `entity:addScript(scriptTable)` &rarr; `void`: Attaches a script table with an `update(self, dt)` function. The engine calls `update` automatically every frame.
* `entity:destroy()` &rarr; `void`: Deactivates and removes the entity and all attached scripts from the active scene.

---

## 📺 Screen System (`engine.Screen`)

Used for switching full-game scenes (e.g., `MenuScreen`, `Level1Screen`, `GameOverScreen`).

| Function | Signature | Description |
| :--- | :--- | :--- |
| `Screen.set(screenTable)` | `(table)` &rarr; `void` | Switches the active screen. Automatically invokes `onExit(self)` on the old screen and `onEnter(self)` on the new screen. |
| `Screen.get()` | `()` &rarr; `table` | Returns the currently active screen table. |

### Screen Lifecycle Table Example

```lua
GameScreen = {
    onEnter = function(self) {
        print("Entered Game Screen")
    },
    update = function(self, dt) {
        -- Screen logic
    },
    draw = function(self) {
        -- Screen rendering
    },
    onExit = function(self) {
        print("Cleaned up Game Screen")
    }
}
```

---

## 🧵 Threaded Workers (`engine.createWorker`)

Launches background OS threads running separate Lua states for non-blocking computation, pathfinding, or continuous audio streaming:

```lua
-- Launch background worker
worker = engine.createWorker("music_worker.lua")

-- Clean up worker when done
engine.destroyWorker(worker)
```
