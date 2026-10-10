/*
** PyLua Engine - Android Stub
** Fallback engine module when running on Android.
*/

#define engine_c
#define LUA_LIB

#include "../include/lua.h"
#include "../include/lauxlib.h"

LUAMOD_API int luaopen_engine(lua_State *L) {
    lua_newtable(L);
    return 1;
}

