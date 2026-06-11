# PyLua

PyLua is a Lua-derived interpreter with a small set of Python-inspired syntax aliases layered on top of the normal Lua grammar.

## Python-style syntax

The original Lua syntax still works. The project also accepts these aliases:

```lua
def add(a, b):
  return a + b
end

if add(1, 1) == 2:
  print("ok")
elif false:
  print("not reached")
else:
  print("fallback")
end

class Greeter:
  def hello(self, name):
    return "hello " .. name
  end
end

print(Greeter.hello(Greeter, "world"))
```

Notes:

- `def name(args): ... end` is sugar for `function name(args) ... end`.
- `class Name: ... end` creates a table named `Name`.
- Bare `def method(args): ... end` statements inside a class body are stored as `Name.method`.
- `if`, `elif`, and `else` support a colon before the block. `elseif` and `then` remain supported.
- `return` remains the canonical spelling; `retun` is accepted as a compatibility alias for misspelled Python-style code.

## Source layout

The C translation units now live in [`src/`](src/) so the repository root is reserved for public headers, tests, documentation, and build metadata.

## Building

```sh
make
./lua -v
```

## GitHub Actions release flow

The workflow in [`.github/workflows/build-release.yml`](.github/workflows/build-release.yml) builds the interpreter on Linux and macOS for every push and pull request. When a tag beginning with `v` is pushed, it also packages the built binary and library and uploads them to a GitHub Release.
