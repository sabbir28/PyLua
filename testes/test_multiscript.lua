-- PyLua Advanced Ecosystem Test
-- Demonstrates: Multi-monitor, Multi-script entities, and Threaded Workers

import engine

print("Starting PyLua Ecosystem...")

-- 1. Multi-Monitor Support
monitors = engine.Window.getMonitors()
print("Available Monitors:")
for i, m in ipairs(monitors) {
    print(string.format("  [%d] %s (%dx%d) at %d,%d", i, m.name, m.width, m.height, m.x, m.y))
}

-- Create window on first monitor
win = engine.Window.create("PyLua Advanced", 1280, 720, false, 1)

-- 2. Multi-Script Entities
-- We'll create a player and attach multiple independent scripts to it
player = engine.Scene.createEntity("Player")

-- Script 1: Movement
movementScript = {
    update = function(self, dt) {
        if (engine.Input.isKeyDown(68)) { -- D
            print("[Player] Moving Right (dt: " .. dt .. ")")
        }
    }
}

-- Script 2: Logging / Health
statsScript = {
    lastLog = os.time(),
    update = function(self, dt) {
        if (os.difftime(os.time(), self.lastLog) >= 2) {
            print("[Player] Status: Healthy")
            self.lastLog = os.time()
        }
    }
}

player.addScript(movementScript)
player.addScript(statsScript)

-- 3. Threaded Workers (Music Ecosystem)
-- This runs on a COMPLETELY SEPARATE thread without hampering the main loop!
print("Starting Background Music Worker...")
worker = engine.createWorker("music_worker.lua")


-- 4. Main Loop
gameConfig = {
    window = win,
    init = function() {
        print("Main Engine Started")
    }
}

engine.run(gameConfig)
print("Main Thread Finished")
