/*
** Cross-platform placeholder for the Windows-only HTTP request library.
*/

#define lrequestlib_c
#define LUA_LIB

#include "../../include/lprefix.h"
#include "../../include/lua.h"
#include "../../include/lauxlib.h"
#include "../../include/lualib.h"

static int request_unavailable(lua_State *L) {
    return luaL_error(L, "the request library is only available on Windows builds");
}

static const luaL_Reg request_funcs[] = {
    {"request", request_unavailable},
    {"get", request_unavailable},
    {"post", request_unavailable},
    {NULL, NULL}
};

LUAMOD_API int luaopen_request(lua_State *L) {
    luaL_newlib(L, request_funcs);
    return 1;
}
