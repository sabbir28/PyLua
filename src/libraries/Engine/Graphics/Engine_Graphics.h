/*
** PyLua Engine - Graphics Subsystem
** Provides 2D rendering primitives for the game engine.
*/

#ifndef ENGINE_GRAPHICS_H
#define ENGINE_GRAPHICS_H

#include "../../../../include/lua.h"

/* Register the Graphics sub-table into the engine table at stack top */
void engine_register_graphics(lua_State *L);

#endif /* ENGINE_GRAPHICS_H */
