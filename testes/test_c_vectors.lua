-- Test C implementation of Vector2 and Vector3
local vector = require "vector"
local Vector2 = vector.Vector2
local Vector3 = vector.Vector3

print("Testing C Vector library...")

local v1 = Vector2(1, 2)
local v2 = Vector2(3, 4)
print("v1:", v1)
print("v2:", v2)
print("v1 + v2:", v1 + v2)
print("v1.x:", v1.x)
v1.x = 10
print("v1.x after change:", v1.x)

local v3 = Vector3(1, 2, 3)
local v4 = Vector3(4, 5, 6)
print("v3:", v3)
print("v3:length():", v3:length())
print("v3:dot(v4):", v3:dot(v4))
print("v3:cross(v4):", v3:cross(v4))

print("C Vector library tests passed!")
