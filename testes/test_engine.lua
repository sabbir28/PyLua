-- Test PyLua Engine Implementation

print("Testing Engine.Window...")
local win = engine.Window.create("PyLua Test", 800, 600)
print("Window created: " .. tostring(win))

print("Testing Engine.Physics...")
local world = engine.Physics.createWorld(0, -9.81)
print("Physics world created: " .. tostring(world))

local body = world:addBody(1, 0, 10) -- Dynamic body at (0, 10)
print("Physics body added: " .. tostring(body))

print("Stepping simulation...")
for i = 1, 5 do
    world:step(0.1)
    local x, y = body:getPosition()
    print(string.format("Step %d: Position = (%.2f, %.2f)", i, x, y))
end

print("Poll window events (will close in a few loops)...")
local count = 0
while win:isOpen() and count < 100 do
    win:pollEvents()
    count = count + 1
end

print("Test complete.")
