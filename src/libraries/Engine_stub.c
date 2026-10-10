/*
** PyLua Engine - Android Stub
** Fallback engine module when running on Android.
*/

#define engine_c
#define LUA_LIB

#include <stdio.h>
#include "../include/lua.h"
#include "../include/lauxlib.h"

int engine_launch_editor(lua_State *L, const char *mode) {
    (void)L;
    (void)mode;
    fprintf(stderr, "PyLua Engine Editor is not supported on Android.\n");
    return 1;
}

LUAMOD_API int luaopen_engine(lua_State *L) {
    lua_newtable(L);
    return 1;
}

