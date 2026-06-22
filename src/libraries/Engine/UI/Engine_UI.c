/*
** PyLua Engine - UI Subsystem
** Implementation of interactive UI components.
*/

#define engine_ui_c
#define LUA_LIB

#include "../../../../include/lprefix.h"
#include <string.h>

#include "../../../../include/lua.h"
#include "../../../../include/lauxlib.h"
#include "Engine_UI.h"

/* ========================================================================= */
/* API Functions                                                             */
/* ========================================================================= */

/*
** engine.UI.button(text, x, y, w, h) -> boolean
** Draws a button and returns true if it was clicked this frame.
** This is an immediate-mode UI helper.
*/
static int ui_button(lua_State *L) {
    const char *text = luaL_checkstring(L, 1);
    float x = (float)luaL_checknumber(L, 2);
    float y = (float)luaL_checknumber(L, 3);
    float w = (float)luaL_checknumber(L, 4);
    float h = (float)luaL_checknumber(L, 5);

    /* 1. Get Mouse State */
    lua_getglobal(L, "engine");
    lua_getfield(L, -1, "Input");
    
    lua_getfield(L, -1, "getMousePos");
    lua_pcall(L, 0, 2, 0);
    float mx = (float)lua_tonumber(L, -2);
    float my = (float)lua_tonumber(L, -1);
    lua_pop(L, 2);

    lua_getfield(L, -1, "isMouseJustPressed");
    lua_pushinteger(L, 1); /* Left button */
    lua_pcall(L, 1, 1, 0);
    int clicked = lua_toboolean(L, -1);
    lua_pop(L, 3); /* pop clicked result, Input, engine */

    /* 2. Check Hover */
    int hover = (mx >= x && mx <= x + w && my >= y && my <= y + h);
    int result = (hover && clicked);

    /* 3. Render */
    lua_getglobal(L, "engine");
    lua_getfield(L, -1, "Graphics");
    
    /* Draw BG */
    lua_getfield(L, -1, "drawRect");
    lua_pushnumber(L, x);
    lua_pushnumber(L, y);
    lua_pushnumber(L, w);
    lua_pushnumber(L, h);
    if (hover) {
        lua_pushnumber(L, 0.4); lua_pushnumber(L, 0.4); lua_pushnumber(L, 0.5); /* Lighter when hover */
    } else {
        lua_pushnumber(L, 0.2); lua_pushnumber(L, 0.2); lua_pushnumber(L, 0.3);
    }
    lua_pushnumber(L, 1.0);
    lua_pcall(L, 8, 0, 0);

    /* Draw Text */
    lua_getfield(L, -1, "drawText");
    lua_pushstring(L, text);
    lua_pushnumber(L, x + 10);
    lua_pushnumber(L, y + (h/2) - 10);
    lua_pushnumber(L, 2.0); /* scale */
    lua_pcall(L, 4, 0, 0);

    lua_pop(L, 2); /* pop Graphics, engine */

    lua_pushboolean(L, result);
    return 1;
}

/*
** engine.UI.panel(x, y, w, h [, r, g, b, a])
*/
static int ui_panel(lua_State *L) {
    float x = (float)luaL_checknumber(L, 1);
    float y = (float)luaL_checknumber(L, 2);
    float w = (float)luaL_checknumber(L, 3);
    float h = (float)luaL_checknumber(L, 4);
    float r = (float)luaL_optnumber(L, 5, 0.1);
    float g = (float)luaL_optnumber(L, 6, 0.1);
    float b = (float)luaL_optnumber(L, 7, 0.1);
    float a = (float)luaL_optnumber(L, 8, 0.8);

    lua_getglobal(L, "engine");
    lua_getfield(L, -1, "Graphics");
    lua_getfield(L, -1, "drawRect");
    lua_pushnumber(L, x);
    lua_pushnumber(L, y);
    lua_pushnumber(L, w);
    lua_pushnumber(L, h);
    lua_pushnumber(L, r);
    lua_pushnumber(L, g);
    lua_pushnumber(L, b);
    lua_pushnumber(L, a);
    lua_pcall(L, 8, 0, 0);
    lua_pop(L, 2);

    return 0;
}

/*
** engine.UI.progressBar(val, max, x, y, w, h)
*/
static int ui_progress_bar(lua_State *L) {
    float val = (float)luaL_checknumber(L, 1);
    float max = (float)luaL_checknumber(L, 2);
    float x = (float)luaL_checknumber(L, 3);
    float y = (float)luaL_checknumber(L, 4);
    float w = (float)luaL_checknumber(L, 5);
    float h = (float)luaL_checknumber(L, 6);

    float pct = val / max;
    if (pct < 0) pct = 0;
    if (pct > 1) pct = 1;

    lua_getglobal(L, "engine");
    lua_getfield(L, -1, "Graphics");
    
    /* BG */
    lua_getfield(L, -1, "drawRect");
    lua_pushnumber(L, x); lua_pushnumber(L, y); lua_pushnumber(L, w); lua_pushnumber(L, h);
    lua_pushnumber(L, 0.1); lua_pushnumber(L, 0.1); lua_pushnumber(L, 0.1); lua_pushnumber(L, 1.0);
    lua_pcall(L, 8, 0, 0);

    /* Fill */
    lua_getfield(L, -1, "drawRect");
    lua_pushnumber(L, x + 2); lua_pushnumber(L, y + 2); lua_pushnumber(L, (w - 4) * pct); lua_pushnumber(L, h - 4);
    lua_pushnumber(L, 0.2); lua_pushnumber(L, 0.8); lua_pushnumber(L, 0.2); lua_pushnumber(L, 1.0);
    lua_pcall(L, 8, 0, 0);

    lua_pop(L, 2);
    return 0;
}

/* ========================================================================= */
/* Registration                                                              */
/* ========================================================================= */

static const luaL_Reg ui_funcs[] = {
    {"button",      ui_button},
    {"panel",       ui_panel},
    {"progressBar", ui_progress_bar},
    {NULL, NULL}
};

void engine_register_ui(lua_State *L) {
    luaL_newlib(L, ui_funcs);
    lua_setfield(L, -2, "UI");
}
