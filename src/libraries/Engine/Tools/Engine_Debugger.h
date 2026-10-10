/*
** PyLua Engine - Visual CPU / Bytecode Debugger
** Interactive disassembler, registers inspector, breakpoints & step execution.
*/

#ifndef ENGINE_DEBUGGER_H
#define ENGINE_DEBUGGER_H

#include "lua.h"

#ifdef __cplusplus
extern "C" {
#endif

int engine_launch_debugger(lua_State *L, const char *script_path, int argc, char **argv);

#ifdef __cplusplus
}
#endif

#endif /* ENGINE_DEBUGGER_H */
