/*
** PyLua Engine - Asset Management
** Implementation of asset loading.
*/

#define engine_assets_c
#define LUA_LIB

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include <GLFW/glfw3.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>

#include "../../../../include/lua.h"
#include "../../../../include/lauxlib.h"
#include "Engine_Assets.h"

#define ENGINE_TEXTURE_MT "Engine.Texture"

/* ========================================================================= */
/* Lua API - Texture                                                         */
/* ========================================================================= */

static int texture_gc(lua_State *L) {
    Texture *tex = (Texture *)luaL_checkudata(L, 1, ENGINE_TEXTURE_MT);
    /* TODO: Delete OpenGL texture if initialized */
    return 0;
}

static int texture_get_size(lua_State *L) {
    Texture *tex = (Texture *)luaL_checkudata(L, 1, ENGINE_TEXTURE_MT);
    lua_pushinteger(L, tex->width);
    lua_pushinteger(L, tex->height);
    return 2;
}

/* ========================================================================= */
/* Assets API                                                                */
/* ========================================================================= */

static int assets_load_texture(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);

    int w, h, c;
    unsigned char *data = stbi_load(path, &w, &h, &c, 0);
    if (!data) {
        return luaL_error(L, "Could not load texture: %s", stbi_failure_reason());
    }

    unsigned int texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);

    /* Set texture parameters */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLint format = (c == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);

    Texture *tex = (Texture *)lua_newuserdatauv(L, sizeof(Texture), 0);
    tex->width = w;
    tex->height = h;
    tex->channels = c;
    tex->id = texture_id;

    luaL_getmetatable(L, ENGINE_TEXTURE_MT);
    lua_setmetatable(L, -2);
    return 1;
}

/* ========================================================================= */
/* Registration                                                              */
/* ========================================================================= */

static const luaL_Reg asset_funcs[] = {
    {"loadTexture", assets_load_texture},
    {NULL, NULL}
};

static const luaL_Reg texture_methods[] = {
    {"getSize", texture_get_size},
    {"__gc", texture_gc},
    {NULL, NULL}
};

void engine_register_assets(lua_State *L) {
    /* Texture Metatable */
    luaL_newmetatable(L, ENGINE_TEXTURE_MT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, texture_methods, 0);
    lua_pop(L, 1);

    /* Create Assets sub-table */
    luaL_newlib(L, asset_funcs);
    lua_setfield(L, -2, "Assets");
}
