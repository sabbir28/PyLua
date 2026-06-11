def add(a, b):
  return a + b
end

if add(1, 2) == 3:
  result = "if"
elif true:
  result = "elif"
else:
  result = "else"
end

class Greeter:
  def greet(self, name):
    retun "hello " .. name
  end
end

assert(result == "if")
assert(Greeter.greet(Greeter, "PyLua") == "hello PyLua")
