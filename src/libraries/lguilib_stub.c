/*
** Cross-platform placeholder for the Windows-only GUI library.
**
** The real implementation lives in lguilib.c and is compiled only for WIN32.
** Keeping a small stub for non-Windows targets lets Linux and Android builds
** expose a clear runtime error instead of failing at compile time because the
** Windows SDK headers are unavailable.
*/

#define lguilib_c
#define LUA_LIB

#include "../../include/lprefix.h"
#include "../../include/lua.h"
#include "../../include/lauxlib.h"
#include "../../include/lualib.h"

static int gui_unavailable(lua_State *L) {
    return luaL_error(L, "the gui library is only available on Windows builds");
}

static const luaL_Reg gui_funcs[] = {
    {"Window", gui_unavailable},
    {NULL, NULL}
};

LUAMOD_API int luaopen_gui(lua_State *L) {
    luaL_newlib(L, gui_funcs);
    return 1;
}
