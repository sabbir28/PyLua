# Lua Auxiliary Library (lauxlib) Modules

This directory contains the modularized fragments of `lauxlib.c`. Each file captures a specific subset of functionality to keep the codebase maintainable and individual file sizes small.

| Fragment | Description |
| :--- | :--- |
| **laux_traceback.c** | Traceback and function name pushing. |
| **laux_error.c** | Error reporting and stack where info. |
| **laux_meta.c** | Metatable and metafield manipulation. |
| **laux_check.c** | Argument and type checking functions. |
| **laux_buffer.c** | Generic buffer implementation. |
| **laux_ref.c** | The Lua reference system. |
| **laux_load.c** | File and buffer loading functions. |
| **laux_misc.c** | Misc functions: require, gsub, alloc, panic, etc. |

These files are included by the main `src/libraries/lauxlib.c` file.
