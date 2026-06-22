/*
** PyLua Engine - Window Subsystem
** Provides window creation and management for the 2D game engine.
*/

#ifndef ENGINE_WINDOW_H
#define ENGINE_WINDOW_H

#include "../../../../include/lua.h"

/* Register the Window sub-table into the engine table at stack top */
void engine_register_window(lua_State *L);

#endif /* ENGINE_WINDOW_H */
