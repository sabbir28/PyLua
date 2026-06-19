local hash = require("hash")
local expected = "9151e1cc7b8d3c486565c433823ae539"
local actual = hash.md5("otdx")

print("Input: otdx")
print("Expected MD5: " .. expected)
print("Actual MD5:   " .. actual)

if actual == expected then
    print("\nSUCCESS: MD5 hash matches!")
else
    print("\nFAILURE: MD5 hash mismatch!")
    os.exit(1)
end

-- Test empty string
local empty_hash = hash.md5("")
print("\nMD5 of empty string: " .. empty_hash)
-- Expected MD5 of empty string: d41d8cd98f00b204e9800998ecf8427e
if empty_hash == "d41d8cd98f00b204e9800998ecf8427e" then
    print("Empty string test passed.")
else
    print("Empty string test failed.")
    os.exit(1)
end
