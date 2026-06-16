/*
** WinAPI GUI library for Lua
** Provides native Windows controls wrapper
*/

#define lguilib_c
#define LUA_LIB

#include "../../include/lprefix.h"

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <string.h>

#include "../../include/lua.h"
#include "../../include/lauxlib.h"
#include "lualib.h"

#define LUA_WINDOW "Window"
#define LUA_CONTROL "Control"

/* Instance handle for the application */
static HINSTANCE g_hInstance = NULL;

typedef struct {
    HWND hwnd;
    int ref_onClick;
} Control;

typedef struct {
    HWND hwnd;
    int is_closed;
} Window;

/* Helper to get Lua state from HWND via UserData */
static lua_State* get_lua_state(HWND hwnd) {
    return (lua_State*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
}

/* Window Procedure */
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    lua_State *L = (lua_State*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    
    switch (msg) {
        case WM_COMMAND: {
            HWND hCtrl = (HWND)lParam;
            if (hCtrl) {
                Control *ctrl = (Control*)GetWindowLongPtr(hCtrl, GWLP_USERDATA);
                if (ctrl && ctrl->ref_onClick != LUA_REFNIL && L) {
                    lua_rawgeti(L, LUA_REGISTRYINDEX, ctrl->ref_onClick);
                    if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
                        fprintf(stderr, "Error in GUI callback: %s\n", lua_tostring(L, -1));
                        lua_pop(L, 1);
                    }
                }
            }
            break;
        }
        case WM_DESTROY: {
            Window *win = (Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA + sizeof(void*));
            if (win) win->is_closed = 1;
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

/* gui.Window(title, w, h) */
static int gui_newwindow(lua_State *L) {
    const char *title = luaL_checkstring(L, 1);
    int w = (int)luaL_optinteger(L, 2, 800);
    int h = (int)luaL_optinteger(L, 3, 600);

    if (!g_hInstance) g_hInstance = GetModuleHandle(NULL);

    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = g_hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "LuaWindowClass";

    RegisterClassEx(&wc);

    HWND hwnd = CreateWindowEx(
        0, "LuaWindowClass", title, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, w, h,
        NULL, NULL, g_hInstance, NULL
    );

    if (!hwnd) return luaL_error(L, "Failed to create window");

    Window *win = (Window*)lua_newuserdatauv(L, sizeof(Window), 0);
    win->hwnd = hwnd;
    win->is_closed = 0;
    
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)L);
    /* For simplicity, store win pointer in another slot or similar. In a real engine, use a map. */
    SetWindowLongPtr(hwnd, GWLP_USERDATA + sizeof(void*), (LONG_PTR)win);

    luaL_getmetatable(L, LUA_WINDOW);
    lua_setmetatable(L, -2);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    
    return 1;
}

static Window *checkwindow(lua_State *L, int arg) {
    return (Window*)luaL_checkudata(L, arg, LUA_WINDOW);
}

/* window:Show() */
static int win_show(lua_State *L) {
    Window *win = checkwindow(L, 1);
    ShowWindow(win->hwnd, SW_SHOW);
    return 0;
}

/* window:Update() */
static int win_update(lua_State *L) {
    Window *win = checkwindow(L, 1);
    MSG msg;
    while (PeekMessage(&msg, win->hwnd, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    lua_pushboolean(L, !win->is_closed);
    return 1;
}

/* window:Button(text, x, y, w, h) */
static int win_button(lua_State *L) {
    Window *win = checkwindow(L, 1);
    const char *text = luaL_checkstring(L, 2);
    int x = (int)luaL_checkinteger(L, 3);
    int y = (int)luaL_checkinteger(L, 4);
    int w = (int)luaL_checkinteger(L, 5);
    int h = (int)luaL_checkinteger(L, 6);

    HWND hBtn = CreateWindow(
        "BUTTON", text, WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        x, y, w, h, win->hwnd, NULL, g_hInstance, NULL
    );

    Control *ctrl = (Control*)lua_newuserdatauv(L, sizeof(Control), 0);
    ctrl->hwnd = hBtn;
    ctrl->ref_onClick = LUA_REFNIL;
    SetWindowLongPtr(hBtn, GWLP_USERDATA, (LONG_PTR)ctrl);
    luaL_getmetatable(L, LUA_CONTROL);
    lua_setmetatable(L, -2);
    return 1;
}

/* window:Label(text, x, y, w, h) */
static int win_label(lua_State *L) {
    Window *win = checkwindow(L, 1);
    const char *text = luaL_checkstring(L, 2);
    int x = (int)luaL_checkinteger(L, 3);
    int y = (int)luaL_checkinteger(L, 4);
    int w = (int)luaL_checkinteger(L, 5);
    int h = (int)luaL_checkinteger(L, 6);

    HWND hStatic = CreateWindow(
        "STATIC", text, WS_VISIBLE | WS_CHILD | SS_LEFT,
        x, y, w, h, win->hwnd, NULL, g_hInstance, NULL
    );

    Control *ctrl = (Control*)lua_newuserdatauv(L, sizeof(Control), 0);
    ctrl->hwnd = hStatic;
    ctrl->ref_onClick = LUA_REFNIL;
    SetWindowLongPtr(hStatic, GWLP_USERDATA, (LONG_PTR)ctrl);
    luaL_getmetatable(L, LUA_CONTROL);
    lua_setmetatable(L, -2);
    return 1;
}

/* window:ScrollBar(x, y, w, h, vertical) */
static int win_scrollbar(lua_State *L) {
    Window *win = checkwindow(L, 1);
    int x = (int)luaL_checkinteger(L, 2);
    int y = (int)luaL_checkinteger(L, 3);
    int w = (int)luaL_checkinteger(L, 4);
    int h = (int)luaL_checkinteger(L, 5);
    int vert = lua_toboolean(L, 6);

    HWND hScroll = CreateWindow(
        "SCROLLBAR", "", WS_VISIBLE | WS_CHILD | (vert ? SBS_VERT : SBS_HORZ),
        x, y, w, h, win->hwnd, NULL, g_hInstance, NULL
    );

    Control *ctrl = (Control*)lua_newuserdatauv(L, sizeof(Control), 0);
    ctrl->hwnd = hScroll;
    ctrl->ref_onClick = LUA_REFNIL;
    SetWindowLongPtr(hScroll, GWLP_USERDATA, (LONG_PTR)ctrl);
    luaL_getmetatable(L, LUA_CONTROL);
    lua_setmetatable(L, -2);
    return 1;
}

/* window:Table(x, y, w, h) */
static int win_table(lua_State *L) {
    Window *win = checkwindow(L, 1);
    int x = (int)luaL_checkinteger(L, 2);
    int y = (int)luaL_checkinteger(L, 3);
    int w = (int)luaL_checkinteger(L, 4);
    int h = (int)luaL_checkinteger(L, 5);

    HWND hLV = CreateWindow(
        WC_LISTVIEW, "", WS_VISIBLE | WS_CHILD | LVS_REPORT | WS_BORDER,
        x, y, w, h, win->hwnd, NULL, g_hInstance, NULL
    );

    Control *ctrl = (Control*)lua_newuserdatauv(L, sizeof(Control), 0);
    ctrl->hwnd = hLV;
    ctrl->ref_onClick = LUA_REFNIL;
    SetWindowLongPtr(hLV, GWLP_USERDATA, (LONG_PTR)ctrl);
    luaL_getmetatable(L, LUA_CONTROL);
    lua_setmetatable(L, -2);
    return 1;
}

/* control:OnClick(callback) */
static int ctrl_onclick(lua_State *L) {
    Control *ctrl = (Control*)luaL_checkudata(L, 1, LUA_CONTROL);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    if (ctrl->ref_onClick != LUA_REFNIL) luaL_unref(L, LUA_REGISTRYINDEX, ctrl->ref_onClick);
    ctrl->ref_onClick = luaL_ref(L, LUA_REGISTRYINDEX);
    return 0;
}

static const luaL_Reg guilib[] = {
    {"Window", gui_newwindow},
    {NULL, NULL}
};

/* window:Frame(text, x, y, w, h) */
static int win_frame(lua_State *L) {
    Window *win = checkwindow(L, 1);
    const char *text = luaL_checkstring(L, 2);
    int x = (int)luaL_checkinteger(L, 3);
    int y = (int)luaL_checkinteger(L, 4);
    int w = (int)luaL_checkinteger(L, 5);
    int h = (int)luaL_checkinteger(L, 6);

    HWND hFrame = CreateWindow(
        "BUTTON", text, WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
        x, y, w, h, win->hwnd, NULL, g_hInstance, NULL
    );

    Control *ctrl = (Control*)lua_newuserdatauv(L, sizeof(Control), 0);
    ctrl->hwnd = hFrame;
    ctrl->ref_onClick = LUA_REFNIL;
    SetWindowLongPtr(hFrame, GWLP_USERDATA, (LONG_PTR)ctrl);
    luaL_getmetatable(L, LUA_CONTROL);
    lua_setmetatable(L, -2);
    return 1;
}

/* window:ToolBar(x, y, w, h) */
static int win_toolbar(lua_State *L) {
    Window *win = checkwindow(L, 1);
    int x = (int)luaL_checkinteger(L, 2);
    int y = (int)luaL_checkinteger(L, 3);
    int w = (int)luaL_checkinteger(L, 4);
    int h = (int)luaL_checkinteger(L, 5);

    HWND hTB = CreateWindowEx(
        0, TOOLBARCLASSNAME, NULL, WS_VISIBLE | WS_CHILD | CCS_TOP,
        x, y, w, h, win->hwnd, NULL, g_hInstance, NULL
    );

    Control *ctrl = (Control*)lua_newuserdatauv(L, sizeof(Control), 0);
    ctrl->hwnd = hTB;
    ctrl->ref_onClick = LUA_REFNIL;
    SetWindowLongPtr(hTB, GWLP_USERDATA, (LONG_PTR)ctrl);
    luaL_getmetatable(L, LUA_CONTROL);
    lua_setmetatable(L, -2);
    return 1;
}

static const luaL_Reg win_methods[] = {
    {"Show", win_show},
    {"Update", win_update},
    {"Button", win_button},
    {"Label", win_label},
    {"ScrollBar", win_scrollbar},
    {"Table", win_table},
    {"Frame", win_frame},
    {"ToolBar", win_toolbar},
    {NULL, NULL}
};

static const luaL_Reg ctrl_methods[] = {
    {"OnClick", ctrl_onclick},
    {NULL, NULL}
};

LUAMOD_API int luaopen_gui (lua_State *L) {
    InitCommonControls();

    /* Window metatable */
    luaL_newmetatable(L, LUA_WINDOW);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, win_methods, 0);
    lua_pop(L, 1);

    /* Control metatable */
    luaL_newmetatable(L, LUA_CONTROL);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, ctrl_methods, 0);
    lua_pop(L, 1);

    luaL_newlib(L, guilib);
    return 1;
}
