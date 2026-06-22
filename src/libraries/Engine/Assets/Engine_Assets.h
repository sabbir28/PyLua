/*
** PyLua Engine - Asset Management
** Handles loading and caching of game assets (textures, sounds, etc.).
*/

#ifndef ENGINE_ASSETS_H
#define ENGINE_ASSETS_H

#include "../../../../include/lua.h"

typedef struct {
    unsigned int id;
    int width;
    int height;
    int channels;
} Texture;

/* Register the Assets sub-table into the engine table at stack top */
void engine_register_assets(lua_State *L);

#endif /* ENGINE_ASSETS_H */
