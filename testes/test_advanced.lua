-- Advanced PyLua Engine Test: Collisions and Sprites

print("Creating Window...")
local win = engine.Window.create("PyLua Advanced Test", 800, 600)

print("Initializing Physics...")
local world = engine.Physics.createWorld(0, -9.81)

-- Create a static floor
local floor = world:addBody(0, 0, 0) -- Static
floor:setRect(10, 1)
print("Floor created.")

-- Create a dynamic falling box
local box = world:addBody(1, 0, 5) -- Dynamic
box:setRect(1, 1)
print("Box created.")

print("Testing Asset Loading...")
-- Note: This requires a 'test.png' to exist. If it doesn't, it will error but we can see the call works.
local status, tex = pcall(engine.Assets.loadTexture, "test.png")
if status then
    print("Texture loaded: " .. tostring(tex))
else
    print("Texture load failed (expected if test.png missing): " .. tostring(tex))
end

print("Simulation loop (100 steps)...")
for i = 1, 100 do
    world:step(0.016) -- ~60 FPS
    if i % 20 == 0 then
        local x, y = box:getPosition()
        print(string.format("Step %d: Box Position = (%.2f, %.2f)", i, x, y))
    end
end

print("Final check: Box should have stopped around y=1.0 (on top of floor)")
local fx, fy = box:getPosition()
print(string.format("Final Position: (%.2f, %.2f)", fx, fy))

print("Test complete.")
