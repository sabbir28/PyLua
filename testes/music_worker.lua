-- PyLua Worker Script: music_worker.lua
-- This script runs on its own thread!

print("[Worker] Music System Initialized")

count = 0
while (count < 10) {
    print("[Worker] Background Music is Playing... (beat " .. count .. ")")
    
    -- Simulate work or waiting
    -- In a real scenario, this would manage audio buffers or network
    startTime = os.time()
    while(os.difftime(os.time(), startTime) < 1) {
        -- busy wait 1s
    }
    
    count = count + 1
}

print("[Worker] Music System Finished")
