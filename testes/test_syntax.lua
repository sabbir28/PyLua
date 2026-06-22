-- PyLua Syntax Test: Import and C-style If

print("Testing 'import'...")
import os
if (os) {
    print("Import 'os' successful!")
    print("OS Type: " .. (os.getenv("OS") or "Unknown"))
}

print("\nTesting 'if' with parentheses and braces...")
local testVar = true
if (testVar) {
    print("Parentheses + braces works!")
}

print("\nTesting 'if' with elseif and braces...")
local value = 10
if (value < 5) {
    print("Should not see this")
}
elseif (value == 10) {
    print("Elseif with braces works!")
}
else {
    print("Should not see this")
}

print("\nTesting backward compatibility (standard Lua if)...")
if true then
    print("Standard 'then/end' still works!")
end

print("\nTesting mixed syntax (paren + then/end)...")
if (true) then
    print("Paren + then/end works!")
end

print("\nTest complete.")
