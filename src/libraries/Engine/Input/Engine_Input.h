/*
** PyLua Engine - Input Subsystem
** Provides keyboard, mouse, and gamepad input for the 2D game engine.
*/

#ifndef ENGINE_INPUT_H
#define ENGINE_INPUT_H

#include "../../../../include/lua.h"

/* Register the Input sub-table into the engine table at stack top */
void engine_register_input(lua_State *L);

#endif /* ENGINE_INPUT_H */
