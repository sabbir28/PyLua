-- test_animator.lua
local engine = require("engine")

local win = engine.Window.create(800, 600, "Animator Multi-Sequence Test")
engine.Graphics.clear(0.1, 0.1, 0.1, 1.0)

-- 1. Load the Animator (it handles the texture and all named sequences)
-- You can create this file in the editor (Tab/N to manage sequences)
local animator = engine.Animation.loadAnimator("animator.panim")

local totalTime = 0
local currentState = "idle"

engine.run({
    window = win,
    update = function(dt)
        totalTime = totalTime + dt
        
        -- Example of programmatic switching
        -- Space key toggles between 'idle' and 'run' (if they exist)
        if engine.Input.isKeyPressed(GLFW_KEY_SPACE) then
            if currentState == "idle" then
                currentState = "run"
            else
                currentState = "idle"
            end
            animator:play(currentState)
            print("Switched to: " .. currentState)
        end
    end,
    draw = function()
        engine.Graphics.clear(0.15, 0.15, 0.18, 1.0)
        
        -- Draw the current animation
        animator:draw(400, 300, totalTime)
        
        engine.Graphics.drawText("State: " .. currentState, 10, 10)
        engine.Graphics.drawText("Press SPACE to toggle State", 10, 40)
    end
})
