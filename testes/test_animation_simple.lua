-- test_animation_simple.lua
local engine = require("engine")

local win = engine.Window.create(800, 600, "Simplified Animation Load")
engine.Graphics.clear(0.1, 0.1, 0.1, 1.0)

-- PROPER WAY: Just load the .panim file!
-- The engine now handles texture loading and sheet creation internally.
local anim = engine.Animation.load("animation.panim", true)

local totalTime = 0

engine.run({
    window = win,
    update = function(dt)
        totalTime = totalTime + dt
    end,
    draw = function()
        engine.Graphics.clear(0.2, 0.2, 0.2, 1.0)
        
        -- Draw the animation at (400, 300) centered
        anim:draw(400, 300, totalTime)
        
        engine.Graphics.drawText("Properly Loaded .panim", 10, 10)
    end
})
