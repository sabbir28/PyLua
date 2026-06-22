-- PyLua Audio Test Script
-- Uses custom PyLua syntax (import, braced if, optional local)

import engine

print("PyLua Audio Test Started")

-- Set master volume
engine.Audio.setVolume(0.8)
print("Master volume set to 0.8")

-- Play a sound (Replace 'test.wav' with a real sound file path)
path = "test.wav" -- AUTO-LOCAL: 'local' is optional for new variables!
print("Attempting to play: " .. path)

channelId = engine.Audio.play(path, 0.5, true, 1.0) -- AUTO-LOCAL

if (channelId > 0) {
    print("Playing on channel: " .. channelId)
} else {
    print("Failed to play sound")
}

-- Wait a bit (simulate a game loop)
startTime = os.time() -- AUTO-LOCAL
paused = false -- AUTO-LOCAL

print("Playing for 10 seconds...")
while (os.difftime(os.time(), startTime) < 10) {
    -- After 3 seconds, pause for 2 seconds
    elapsed = os.difftime(os.time(), startTime) -- AUTO-LOCAL in loop
    
    if (elapsed > 3 and elapsed < 5 and not paused) {
        print("Pausing playback...")
        engine.Audio.pause(channelId)
        paused = true
    }
    
    if (elapsed >= 5 and paused) {
        print("Resuming playback...")
        engine.Audio.resume(channelId)
        paused = false
    }
}

print("Stopping playback")
engine.Audio.stop(channelId)

print("Test Finished")
