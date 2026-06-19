-- test_request.lua
local request = require "request"

print("Starting HTTP Request Test...")

local url = "https://jsonplaceholder.typicode.com/posts/1"
print("GET " .. url)

local res = request.get(url)

print("Status Code:", res.status_code)
print("Response Body (excerpt):", string.sub(res.text, 1, 100))

if res.status_code == 200 then
    print("SUCCESS: Received expected status code.")
else
    print("FAILURE: Received status code " .. res.status_code)
end

print("\nFull Headers (Raw):")
print(res.headers_raw)

print("\nRequest test finished.")
