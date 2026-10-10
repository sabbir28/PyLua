/*
** PyLua Visual Bytecode / CPU Debugger PRO
** Module: Debugger UI & Window Layout
*/

#ifndef DEBUGGER_UI_H
#define DEBUGGER_UI_H

#include "Debugger_Types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Main Debugger Window Render Pass */
void render_debugger_gui(lua_State *L);

#ifdef __cplusplus
}
#endif

#endif /* DEBUGGER_UI_H */
