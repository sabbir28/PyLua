-- test_emu.lua
emu player
{
  running,
  sleeping,
  walking
}

print("player.running:", player.running)
print("player.sleeping:", player.sleeping)
print("player.walking:", player.walking)

assert(player.running == 0)
assert(player.sleeping == 1)
assert(player.walking == 2)

print("Enum 'emu' test passed!")
