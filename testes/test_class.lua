-- test_class.lua

class Vec2 {
    x = 0,
    y = 0,

    function new(self, x, y) {
        self.x = x
        self.y = y
        return setmetatable(self, Vec2)
    }

    function length(self) {
        return math.sqrt(self.x * self.x + self.y * self.y)
    }

    function tostring(self) {
        return "Vec2(" .. self.x .. ", " .. self.y .. ")"
    }
}

local v = Vec2:new(3, 4)
print(v:tostring())
assert(v:length() == 5, "Expected length 5")
print("class test passed!")
