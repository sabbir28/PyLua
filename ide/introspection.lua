-- ide/introspection.lua
-- Enhanced bridge script for PyLua IDE Support

local target_expr = arg[1]
if not target_expr then 
    -- List all globals if no target
    for k, v in pairs(_G) do
        print(k .. ":" .. type(v))
    end
    return 
end

-- Safely evaluate the target expression
-- We use load() to allow "require('lib').member" or "Class.method" chain
local function evaluate(expr)
    local f = load("return " .. expr)
    if f then
        local ok, res = pcall(f)
        if ok then return res end
    end
    -- Fallback to global/require search if direct load fails
    local obj = _G[expr]
    if not obj then
        local ok, res = pcall(require, expr)
        if ok then obj = res end
    end
    return obj
end

local obj = evaluate(target_expr)

if type(obj) == "table" then
    for k, v in pairs(obj) do
        print(string.format("%s:%s", k, type(v)))
    end
elseif type(obj) == "userdata" then
    local mt = getmetatable(obj)
    if mt and type(mt.__index) == "table" then
        for k, v in pairs(mt.__index) do
            print(string.format("%s:%s", k, type(v)))
        end
    end
elseif type(obj) == "function" then
    -- Special case for functions if needed
    print("__call:function")
end
