-- test_animation.lua
local engine = require("engine")

local win = engine.Window.create(800, 600, "Animation Test")
engine.Graphics.clear(0.1, 0.1, 0.1, 1.0)

-- 1. Load a texture (Requires a 'test_sheet.png' in assets/)
-- For the sake of this test, we assume the user has one. 
-- In a real scenario, the editor would have created assets.
local tex = engine.Assets.loadTexture("assets/test_sheet.png")

-- 2. Create a SpriteSheet (64x64 frames)
local sheet = engine.Animation.newSpriteSheet(tex, 64, 64)

-- 3. Create an Animation using Keyframes
-- frame 0 for 0.2s, frame 1 for 0.1s, frame 2 for 0.5s (dramatic pause!)
local anim = engine.Animation.new(sheet, {
    {frame = 0, duration = 0.2},
    {frame = 1, duration = 0.1},
    {frame = 2, duration = 0.5},
    {frame = 1, duration = 0.1}
}, true)

-- 4. Or load from a file created in the editor
-- local anim2 = engine.Animation.load(sheet, "animation.panim", true)

local totalTime = 0

engine.run({
    window = win,
    update = function(dt)
        totalTime = totalTime + dt
    end,
    draw = function()
        engine.Graphics.clear(0.2, 0.2, 0.2, 1.0)
        
        -- Draw the animation at (100, 100)
        anim:draw(100, 100, totalTime)
        
        engine.Graphics.drawText("Animation Playing...", 10, 10)
    end
})
