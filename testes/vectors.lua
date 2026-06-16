-- Vector module for game engine
-- Provides 2D and 3D vector implementations

local Vector2 = {}
Vector2.__index = Vector2

function Vector2.new(x, y)
    return setmetatable({x = x or 0, y = y or 0}, Vector2)
end

function Vector2.__add(a, b)
    return Vector2.new(a.x + b.x, a.y + b.y)
end

function Vector2.__sub(a, b)
    return Vector2.new(a.x - b.x, a.y - b.y)
end

function Vector2.__mul(a, b)
    if type(b) == "number" then
        return Vector2.new(a.x * b, a.y * b)
    elseif type(a) == "number" then
        return Vector2.new(b.x * a, b.y * a)
    else
        return Vector2.new(a.x * b.x, a.y * b.y)
    end
end

function Vector2.__div(a, b)
    if type(b) == "number" then
        return Vector2.new(a.x / b, a.y / b)
    else
        return Vector2.new(a.x / b.x, a.y / b.y)
    end
end

function Vector2.__eq(a, b)
    return a.x == b.x and a.y == b.y
end

function Vector2.__tostring(v)
    return string.format("Vector2(%g, %g)", v.x, v.y)
end

function Vector2:length_sq()
    return self.x * self.x + self.y * self.y
end

function Vector2:length()
    return math.sqrt(self:length_sq())
end

function Vector2:normalize()
    local len = self:length()
    if len > 0 then
        return self / len
    end
    return Vector2.new(0, 0)
end

function Vector2:dot(other)
    return self.x * other.x + self.y * other.y
end

function Vector2:dist_sq(other)
    return (self.x - other.x)^2 + (self.y - other.y)^2
end

function Vector2:dist(other)
    return math.sqrt(self:dist_sq(other))
end


local Vector3 = {}
Vector3.__index = Vector3

function Vector3.new(x, y, z)
    return setmetatable({x = x or 0, y = y or 0, z = z or 0}, Vector3)
end

function Vector3.__add(a, b)
    return Vector3.new(a.x + b.x, a.y + b.y, a.z + b.z)
end

function Vector3.__sub(a, b)
    return Vector3.new(a.x - b.x, a.y - b.y, a.z - b.z)
end

function Vector3.__mul(a, b)
    if type(b) == "number" then
        return Vector3.new(a.x * b, a.y * b, a.z * b)
    elseif type(a) == "number" then
        return Vector3.new(b.x * a, b.y * a, b.z * a)
    else
        return Vector3.new(a.x * b.x, a.y * b.y, a.z * b.z)
    end
end

function Vector3.__div(a, b)
    if type(b) == "number" then
        return Vector3.new(a.x / b, a.y / b, a.z / b)
    else
        return Vector3.new(a.x / b.x, a.y / b.y, a.z / b.z)
    end
end

function Vector3.__eq(a, b)
    return a.x == b.x and a.y == b.y and a.z == b.z
end

function Vector3.__tostring(v)
    return string.format("Vector3(%g, %g, %g)", v.x, v.y, v.z)
end

function Vector3:length_sq()
    return self.x * self.x + self.y * self.y + self.z * self.z
end

function Vector3:length()
    return math.sqrt(self:length_sq())
end

function Vector3:normalize()
    local len = self:length()
    if len > 0 then
        return self / len
    end
    return Vector3.new(0, 0, 0)
end

function Vector3:dot(other)
    return self.x * other.x + self.y * other.y + self.z * other.z
end

function Vector3:cross(other)
    return Vector3.new(
        self.y * other.z - self.z * other.y,
        self.z * other.x - self.x * other.z,
        self.x * other.y - self.y * other.x
    )
end

function Vector3:dist_sq(other)
    return (self.x - other.x)^2 + (self.y - other.y)^2 + (self.z - other.z)^2
end

function Vector3:dist(other)
    return math.sqrt(self:dist_sq(other))
end

-- Export the constructors
return {
    Vector2 = Vector2.new,
    Vector3 = Vector3.new
}
