/*
** PyLua Engine - UI Subsystem
** Provides interactive UI components like buttons, panels, and progression bars.
*/

#ifndef ENGINE_UI_H
#define ENGINE_UI_H

#include "../../../../include/lua.h"

/* Register the UI sub-table into the engine table at stack top */
void engine_register_ui(lua_State *L);

#endif /* ENGINE_UI_H */
