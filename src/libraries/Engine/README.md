# PyLua Engine API Documentation

Welcome to the **PyLua Engine** API documentation. This engine provides a robust suite of tools for 2D game development, featuring a Unity-inspired game loop, entity-script architecture, and parallel worker threading.

---

## 🚀 Getting Started

To use the engine, import it at the top of your script:
```lua
import engine
```

---

## 🏛️ Main Engine API (`engine`)

| Function | Description |
| :--- | :--- |
| `run(config)` | Starts the main game loop with a config table (see below). |
| `stop()` | Signals the engine to stop running on the next frame. |
| `getDeltaTime()` | Returns the time in seconds between the current and last frame. |
| `createWorker(path)`| Starts a new background Lua thread (Worker) for parallel tasks. |
| `destroyWorker(worker)`| Cleans up and removes a background worker. |

### `engine.run(config)` Table Structure:
```lua
config = {
    window = windowObj,      -- Required
    physicsWorld = world,    -- Optional: Auto-steps physics
    init = function(),       -- Called once at start
    update = function(dt),   -- Called every frame for logic
    draw = function()        -- Called every frame for rendering
}
```

---

## 📂 Scene & Entity System (`engine.Scene`)

The ecosystem where all game objects live.

| Function | Description |
| :--- | :--- |
| `Scene.createEntity(name)` | Creates and returns a new game Entity. |

---

## 📺 Screen System (`engine.Screen`)

Used for switching between full-game states (Menus, Level 1, etc.).

| Function | Description |
| :--- | :--- |
| `Screen.set(table)` | Switches to a new screen. Calls `onExit` and `onEnter`. |
| `Screen.get()` | Returns the current screen table. |

### Screen Table Lifecycle:
```lua
MyScreen = {
    onEnter = function(self),  -- Optional: Called when switching to this screen
    update = function(self, dt), -- Optional: Called every frame
    draw = function(self),     -- Optional: Called every frame
    onExit = function(self)    -- Optional: Called when switching out
}
```

---

## 📦 Entity Methods:
| Method | Description |
| :--- | :--- |
| `entity:addScript(script)` | Attaches a script table with an `update(self, dt)` function. |
| `entity:destroy()` | Removes the entity and its scripts from the scene. |

---

## 🖼️ Window Subsystem (`engine.Window`)

| Function | Description |
| :--- | :--- |
| `create(title, w, h, fs, mon)` | Creates a window. `fs` is fullscreen, `mon` is monitor index. |
| `getMonitors()` | Returns a list of all detected monitors. |

### Window Instance Methods:
- `isOpen()`, `close()`, `pollEvents()`, `getSize()`, `setSize(w,h)`, `setTitle(t)`.

---

## 🎨 Graphics Subsystem (`engine.Graphics`)

| Function | Description |
| :--- | :--- |
| `clear(r, g, b, a)` | Clears the screen with the specified color. |
| `drawRect(x, y, w, h, r, g, b, a)` | Draws a colored rectangle. |
| `drawSprite(assetId, x, y, w, h)` | Draws a sprite from the Assets system. |

---

## ⚖️ Physics Subsystem (`engine.Physics`)

| Function | Description |
| :--- | :--- |
| `createWorld(gx, gy)` | Creates a physics world with gravity (gx, gy). |

### World Methods:
- `addBody(type, x, y)`: `type`: 1 = Dynamic, 0 = Static. Returns a Body.
- `step(dt)`: Manually step the simulation (if not used in `engine.run`).

### Body Methods:
- `getPosition()`, `setVelocity(vx, vy)`, `setCircle(radius)`, `setRect(w, h)`.

---

## 🔊 Audio Subsystem (`engine.Audio`)

| Function | Description |
| :--- | :--- |
| `play(path, vol, loop, pitch)` | Plays a sound file. Returns a channel ID. |
| `stop(id)` | Stops playback on a specific channel (or all if ID is nil). |
| `setVolume(vol)` | Sets master volume (0.0 - 1.0). |

---

## 🖥️ UI Subsystem (`engine.UI`)

| Function | Description |
| :--- | :--- |
| `button(text, x, y, w, h)` | Draws a button. Returns `true` if clicked this frame. |
| `panel(x, y, w, h [, r, g, b, a])` | Draws a background panel. |
| `progressBar(val, max, x, y, w, h)`| Draws a progress/health bar. |

---

## ⌨️ Input Subsystem (`engine.Input`)

| Function | Description |
| :--- | :--- |
| `isKeyDown(key)` | Returns true if the key is currently pressed. |
| `getMousePos()` | Returns `x, y` mouse coordinates. |

---

## 🛠️ Assets Subsystem (`engine.Assets`)

| Function | Description |
| :--- | :--- |
| `loadTexture(path)` | Loads an image and returns an asset ID. |
