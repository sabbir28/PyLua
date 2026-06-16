-- GUI Basics Demo
local gui = require "gui"

print("Starting GUI Demo...")

local win = gui.Window("PyLua GUI Showcase", 600, 450)

-- Header / Label
win:Label("Native Win32 GUI Library for Lua", 10, 10, 300, 20)

-- Frame / GroupBox
local frame = win:Frame("Actions", 10, 40, 580, 100)

-- Buttons
local btn1 = win:Button("Click Me!", 20, 60, 100, 30)
btn1:OnClick(function()
    print("Button 1 Clicked!")
end)

local btn2 = win:Button("Close App", 130, 60, 100, 30)
btn2:OnClick(function()
    print("Closing...")
    os.exit()
end)

-- ScrollBars
win:Label("Volume:", 10, 150, 60, 20)
win:ScrollBar(80, 150, 200, 20, false) -- Horizontal

-- Table (ListView)
win:Label("Process List:", 10, 180, 100, 20)
local table = win:Table(10, 200, 560, 150)

-- Toolbar
win:ToolBar(0, 360, 600, 40)

print("GUI Window created. Pump message loop...")

-- Message Loop
while win:Update() do
    -- Idle or perform other tasks
end

print("GUI window closed.")
