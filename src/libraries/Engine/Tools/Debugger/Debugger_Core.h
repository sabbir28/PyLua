/*
** PyLua Visual Bytecode / CPU Debugger PRO
** Module: Debugger Core & State Management
*/

#ifndef DEBUGGER_CORE_H
#define DEBUGGER_CORE_H

#include "Debugger_Types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Global Debugger State */
extern DebuggerState g_dbg;

/* State lifecycle */
void dbg_state_init(GLFWwindow *window, const char *script_path);
void dbg_state_cleanup(void);

/* Logging */
void dbg_log_add(int level, const char *fmt, ...);
int dbg_custom_print(lua_State *L);

/* Source Script Management */
void load_script_source(const char *path);
void free_script_source(void);

/* Breakpoints */
int find_breakpoint(int pc);
void toggle_breakpoint(int pc, int line);
void clear_breakpoints(void);

/* Call Stack */
int get_call_depth(lua_State *L);

#ifdef __cplusplus
}
#endif

#endif /* DEBUGGER_CORE_H */
