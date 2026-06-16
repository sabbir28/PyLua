-- Tests for Vector2 and Vector3
local vectors = require "vectors"
local Vector2 = vectors.Vector2
local Vector3 = vectors.Vector3

local function assert_eq(a, b, msg)
    if a ~= b then
        error(string.format("%s: expected %s, got %s", msg or "assertion failed", tostring(b), tostring(a)))
    end
end

local function assert_near(a, b, epsilon, msg)
    epsilon = epsilon or 1e-6
    if math.abs(a - b) > epsilon then
        error(string.format("%s: expected %s (near %s), got %s", msg or "assertion failed", tostring(b), tostring(b), tostring(a)))
    end
end

print("Testing Vector2...")
do
    local v1 = Vector2(1, 2)
    local v2 = Vector2(3, 4)
    
    -- Basic arithmetic
    local v3 = v1 + v2
    assert_eq(v3, Vector2(4, 6), "Vector2 addition")
    
    local v4 = v2 - v1
    assert_eq(v4, Vector2(2, 2), "Vector2 subtraction")
    
    local v5 = v1 * 2
    assert_eq(v5, Vector2(2, 4), "Vector2 scalar multiplication")
    
    local v6 = v2 / 2
    assert_eq(v6, Vector2(1.5, 2), "Vector2 scalar division")
    
    local v7 = v1 * v2
    assert_eq(v7, Vector2(3, 8), "Vector2 element-wise multiplication")
    
    -- Properties
    assert_near(Vector2(3, 4):length(), 5, 1e-6, "Vector2 length")
    assert_near(Vector2(3, 4):length_sq(), 25, 1e-6, "Vector2 length_sq")
    
    local v8 = Vector2(3, 0):normalize()
    assert_eq(v8, Vector2(1, 0), "Vector2 normalize")
    
    assert_eq(v1:dot(v2), 1*3 + 2*4, "Vector2 dot product")
    assert_near(v1:dist(v2), math.sqrt((1-3)^2 + (2-4)^2), 1e-6, "Vector2 distance")
end

print("Testing Vector3...")
do
    local v1 = Vector3(1, 2, 3)
    local v2 = Vector3(4, 5, 6)
    
    -- Basic arithmetic
    local v3 = v1 + v2
    assert_eq(v3, Vector3(5, 7, 9), "Vector3 addition")
    
    local v4 = v2 - v1
    assert_eq(v4, Vector3(3, 3, 3), "Vector3 subtraction")
    
    local v5 = v1 * 2
    assert_eq(v5, Vector3(2, 4, 6), "Vector3 scalar multiplication")
    
    local v6 = v2 / 2
    assert_eq(v6, Vector3(2, 2.5, 3), "Vector3 scalar division")
    
    local v7 = v1 * v2
    assert_eq(v7, Vector3(4, 10, 18), "Vector3 element-wise multiplication")
    
    -- Properties
    assert_near(Vector3(1, 2, 2):length(), 3, 1e-6, "Vector3 length")
    
    local v8 = Vector3(0, 5, 0):normalize()
    assert_eq(v8, Vector3(0, 1, 0), "Vector3 normalize")
    
    assert_eq(v1:dot(v2), 1*4 + 2*5 + 3*6, "Vector3 dot product")
    
    -- Cross product
    local cross = Vector3(1, 0, 0):cross(Vector3(0, 1, 0))
    assert_eq(cross, Vector3(0, 0, 1), "Vector3 cross product")
    
    assert_near(v1:dist(v2), math.sqrt((1-4)^2 + (2-5)^2 + (3-6)^2), 1e-6, "Vector3 distance")
end

print("All Vector tests passed!")
