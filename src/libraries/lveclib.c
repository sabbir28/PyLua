/*
** Vector library for Lua
** Provides Vector2 and Vector3 types
*/

#define lveclib_c
#define LUA_LIB

#include "../../include/lprefix.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../include/lua.h"
#include "../../include/lauxlib.h"
#include "lualib.h"

#define LUA_VECTOR2 "Vector2"
#define LUA_VECTOR3 "Vector3"

typedef struct {
    lua_Number x, y;
} Vector2;

typedef struct {
    lua_Number x, y, z;
} Vector3;

/*
** Vector2 implementation
*/

static int vec2_new(lua_State *L) {
    lua_Number x = luaL_optnumber(L, 1, 0);
    lua_Number y = luaL_optnumber(L, 2, 0);
    Vector2 *v = (Vector2 *)lua_newuserdatauv(L, sizeof(Vector2), 0);
    v->x = x;
    v->y = y;
    luaL_getmetatable(L, LUA_VECTOR2);
    lua_setmetatable(L, -2);
    return 1;
}

static Vector2 *checkvec2(lua_State *L, int arg) {
    return (Vector2 *)luaL_checkudata(L, arg, LUA_VECTOR2);
}

static int vec2_add(lua_State *L) {
    Vector2 *a = checkvec2(L, 1);
    Vector2 *b = checkvec2(L, 2);
    lua_pushnumber(L, a->x + b->x);
    lua_pushnumber(L, a->y + b->y);
    return vec2_new(L);
}

static int vec2_sub(lua_State *L) {
    Vector2 *a = checkvec2(L, 1);
    Vector2 *b = checkvec2(L, 2);
    lua_pushnumber(L, a->x - b->x);
    lua_pushnumber(L, a->y - b->y);
    return vec2_new(L);
}

static int vec2_mul(lua_State *L) {
    if (lua_isnumber(L, 2)) {
        Vector2 *a = checkvec2(L, 1);
        lua_Number s = lua_tonumber(L, 2);
        lua_pushnumber(L, a->x * s);
        lua_pushnumber(L, a->y * s);
    } else {
        Vector2 *a = checkvec2(L, 1);
        Vector2 *b = checkvec2(L, 2);
        lua_pushnumber(L, a->x * b->x);
        lua_pushnumber(L, a->y * b->y);
    }
    return vec2_new(L);
}

static int vec2_div(lua_State *L) {
    Vector2 *a = checkvec2(L, 1);
    lua_Number s = luaL_checknumber(L, 2);
    lua_pushnumber(L, a->x / s);
    lua_pushnumber(L, a->y / s);
    return vec2_new(L);
}

static int vec2_eq(lua_State *L) {
    Vector2 *a = checkvec2(L, 1);
    Vector2 *b = checkvec2(L, 2);
    lua_pushboolean(L, (a->x == b->x && a->y == b->y));
    return 1;
}

static int vec2_tostring(lua_State *L) {
    Vector2 *v = checkvec2(L, 1);
    lua_pushfstring(L, "Vector2(%f, %f)", (double)v->x, (double)v->y);
    return 1;
}

static int vec2_len(lua_State *L) {
    Vector2 *v = checkvec2(L, 1);
    lua_pushnumber(L, sqrt(v->x * v->x + v->y * v->y));
    return 1;
}

static int vec2_dot(lua_State *L) {
    Vector2 *a = checkvec2(L, 1);
    Vector2 *b = checkvec2(L, 2);
    lua_pushnumber(L, a->x * b->x + a->y * b->y);
    return 1;
}

static int vec2_index(lua_State *L) {
    Vector2 *v = checkvec2(L, 1);
    const char *key = luaL_checkstring(L, 2);
    if (strcmp(key, "x") == 0) lua_pushnumber(L, v->x);
    else if (strcmp(key, "y") == 0) lua_pushnumber(L, v->y);
    else {
        luaL_getmetatable(L, LUA_VECTOR2);
        lua_pushvalue(L, 2);
        lua_gettable(L, -2);
    }
    return 1;
}

static int vec2_newindex(lua_State *L) {
    Vector2 *v = checkvec2(L, 1);
    const char *key = luaL_checkstring(L, 2);
    lua_Number val = luaL_checknumber(L, 3);
    if (strcmp(key, "x") == 0) v->x = val;
    else if (strcmp(key, "y") == 0) v->y = val;
    else luaL_error(L, "Vector2 has no field '%s'", key);
    return 0;
}

/*
** Vector3 implementation
*/

static int vec3_new(lua_State *L) {
    lua_Number x = luaL_optnumber(L, 1, 0);
    lua_Number y = luaL_optnumber(L, 2, 0);
    lua_Number z = luaL_optnumber(L, 3, 0);
    Vector3 *v = (Vector3 *)lua_newuserdatauv(L, sizeof(Vector3), 0);
    v->x = x;
    v->y = y;
    v->z = z;
    luaL_getmetatable(L, LUA_VECTOR3);
    lua_setmetatable(L, -2);
    return 1;
}

static Vector3 *checkvec3(lua_State *L, int arg) {
    return (Vector3 *)luaL_checkudata(L, arg, LUA_VECTOR3);
}

static int vec3_add(lua_State *L) {
    Vector3 *a = checkvec3(L, 1);
    Vector3 *b = checkvec3(L, 2);
    lua_pushnumber(L, a->x + b->x);
    lua_pushnumber(L, a->y + b->y);
    lua_pushnumber(L, a->z + b->z);
    return vec3_new(L);
}

static int vec3_sub(lua_State *L) {
    Vector3 *a = checkvec3(L, 1);
    Vector3 *b = checkvec3(L, 2);
    lua_pushnumber(L, a->x - b->x);
    lua_pushnumber(L, a->y - b->y);
    lua_pushnumber(L, a->z - b->z);
    return vec3_new(L);
}

static int vec3_mul(lua_State *L) {
    if (lua_isnumber(L, 2)) {
        Vector3 *a = checkvec3(L, 1);
        lua_Number s = lua_tonumber(L, 2);
        lua_pushnumber(L, a->x * s);
        lua_pushnumber(L, a->y * s);
        lua_pushnumber(L, a->z * s);
    } else {
        Vector3 *a = checkvec3(L, 1);
        Vector3 *b = checkvec3(L, 2);
        lua_pushnumber(L, a->x * b->x);
        lua_pushnumber(L, a->y * b->y);
        lua_pushnumber(L, a->z * b->z);
    }
    return vec3_new(L);
}

static int vec3_div(lua_State *L) {
    Vector3 *a = checkvec3(L, 1);
    lua_Number s = luaL_checknumber(L, 2);
    lua_pushnumber(L, a->x / s);
    lua_pushnumber(L, a->y / s);
    lua_pushnumber(L, a->z / s);
    return vec3_new(L);
}

static int vec3_eq(lua_State *L) {
    Vector3 *a = checkvec3(L, 1);
    Vector3 *b = checkvec3(L, 2);
    lua_pushboolean(L, (a->x == b->x && a->y == b->y && a->z == b->z));
    return 1;
}

static int vec3_tostring(lua_State *L) {
    Vector3 *v = checkvec3(L, 1);
    lua_pushfstring(L, "Vector3(%f, %f, %f)", (double)v->x, (double)v->y, (double)v->z);
    return 1;
}

static int vec3_len(lua_State *L) {
    Vector3 *v = checkvec3(L, 1);
    lua_pushnumber(L, sqrt(v->x * v->x + v->y * v->y + v->z * v->z));
    return 1;
}

static int vec3_dot(lua_State *L) {
    Vector3 *a = checkvec3(L, 1);
    Vector3 *b = checkvec3(L, 2);
    lua_pushnumber(L, a->x * b->x + a->y * b->y + a->z * b->z);
    return 1;
}

static int vec3_cross(lua_State *L) {
    Vector3 *a = checkvec3(L, 1);
    Vector3 *b = checkvec3(L, 2);
    lua_pushnumber(L, a->y * b->z - a->z * b->y);
    lua_pushnumber(L, a->z * b->x - a->x * b->z);
    lua_pushnumber(L, a->x * b->y - a->y * b->x);
    return vec3_new(L);
}

static int vec3_index(lua_State *L) {
    Vector3 *v = checkvec3(L, 1);
    const char *key = luaL_checkstring(L, 2);
    if (strcmp(key, "x") == 0) lua_pushnumber(L, v->x);
    else if (strcmp(key, "y") == 0) lua_pushnumber(L, v->y);
    else if (strcmp(key, "z") == 0) lua_pushnumber(L, v->z);
    else {
        luaL_getmetatable(L, LUA_VECTOR3);
        lua_pushvalue(L, 2);
        lua_gettable(L, -2);
    }
    return 1;
}

static int vec3_newindex(lua_State *L) {
    Vector3 *v = checkvec3(L, 1);
    const char *key = luaL_checkstring(L, 2);
    lua_Number val = luaL_checknumber(L, 3);
    if (strcmp(key, "x") == 0) v->x = val;
    else if (strcmp(key, "y") == 0) v->y = val;
    else if (strcmp(key, "z") == 0) v->z = val;
    else luaL_error(L, "Vector3 has no field '%s'", key);
    return 0;
}

static const luaL_Reg vec2_methods[] = {
    {"length", vec2_len},
    {"dot", vec2_dot},
    {NULL, NULL}
};

static const luaL_Reg vec2_metamethods[] = {
    {"__add", vec2_add},
    {"__sub", vec2_sub},
    {"__mul", vec2_mul},
    {"__div", vec2_div},
    {"__eq", vec2_eq},
    {"__tostring", vec2_tostring},
    {"__index", vec2_index},
    {"__newindex", vec2_newindex},
    {NULL, NULL}
};

static const luaL_Reg vec3_methods[] = {
    {"length", vec3_len},
    {"dot", vec3_dot},
    {"cross", vec3_cross},
    {NULL, NULL}
};

static const luaL_Reg vec3_metamethods[] = {
    {"__add", vec3_add},
    {"__sub", vec3_sub},
    {"__mul", vec3_mul},
    {"__div", vec3_div},
    {"__eq", vec3_eq},
    {"__tostring", vec3_tostring},
    {"__index", vec3_index},
    {"__newindex", vec3_newindex},
    {NULL, NULL}
};

static const luaL_Reg veclib[] = {
    {"Vector2", vec2_new},
    {"Vector3", vec3_new},
    {NULL, NULL}
};

LUAMOD_API int luaopen_vector (lua_State *L) {
    luaL_newmetatable(L, LUA_VECTOR2);
    luaL_setfuncs(L, vec2_metamethods, 0);
    luaL_newlib(L, vec2_methods);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);

    luaL_newmetatable(L, LUA_VECTOR3);
    luaL_setfuncs(L, vec3_metamethods, 0);
    luaL_newlib(L, vec3_methods);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);

    luaL_newlib(L, veclib);
    return 1;
}

