-- PyLua Professional Game Test
-- Demonstrates the engine.run() loop with Unity-style callbacks

import engine

print("Initializing Game...")

-- 1. Setup Window
win = engine.Window.create("PyLua 2D Game", 1280, 720)

-- 2. Setup Physics
world = engine.Physics.createWorld(0, -500) -- Gravity down

-- 3. Create Player (using Physics)
player = world.addBody(1, 640, 360) -- Dynamic body at center
player.setCircle(30)
player.setVelocity(0, 0)

-- 4. Game Logic
playerSpeed = 300

gameConfig = {
    window = win,
    physicsWorld = world,
    
    init = function() {
        print("Game Started!")
    },
    
    update = function(dt) {
        -- Input handling (Python-style / optional local)
        moveX = 0
        moveY = 0
        
        if (engine.Input.isKeyDown(32)) { -- Space to jump
            player.setVelocity(0, 500)
        }
        
        if (engine.Input.isKeyDown(65)) { moveX = -1 } -- A
        if (engine.Input.isKeyDown(68)) { moveX = 1 }  -- D
        
        -- Apply movement
        if (moveX != 0) {
            x, y = player.getPosition()
            player.setVelocity(moveX * playerSpeed, 0)
        }
    },
    
    draw = function() {
        -- Clear screen with a nice dark color
        engine.Graphics.clear(0.1, 0.1, 0.15, 1.0)
        
        -- Draw the player
        x, y = player.getPosition()
        engine.Graphics.drawRect(x - 30, y - 30, 60, 60, 0.0, 0.8, 0.0, 1.0)
    }
}

-- Start the engine!
print("Entering Game Loop...")
engine.run(gameConfig)

print("Game Over")
