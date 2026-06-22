/*
** PyLua Engine - Input Subsystem
** Provides keyboard, mouse, and gamepad input for the 2D game engine.
*/

#define engine_input_c
#define LUA_LIB

#include "../../../../include/lprefix.h"

#include <string.h>

#include "../../../../include/lua.h"
#include "../../../../include/lauxlib.h"
#include "Engine_Input.h"

/* ========================================================================= */
/* Internal State                                                            */
/* ========================================================================= */

#define MAX_KEYS 256
#define MAX_MOUSE_BUTTONS 8

static struct {
    int keys[MAX_KEYS];          /* 1 = pressed, 0 = released */
    int keys_prev[MAX_KEYS];     /* previous frame state (for justPressed) */
    double mouse_x, mouse_y;
    int mouse_buttons[MAX_MOUSE_BUTTONS];
    int mouse_buttons_prev[MAX_MOUSE_BUTTONS];
    double scroll_x, scroll_y;
} input_state;

/* Map a key name string to an index (simplified) */
static int key_name_to_index(const char *name) {
    /* Letters a-z */
    if (name[0] != '\0' && name[1] == '\0') {
        char c = name[0];
        if (c >= 'a' && c <= 'z') return c - 'a';
        if (c >= 'A' && c <= 'Z') return (c - 'A');
        if (c >= '0' && c <= '9') return 26 + (c - '0');
    }
    /* Special keys */
    if (strcmp(name, "space") == 0)  return 36;
    if (strcmp(name, "enter") == 0)  return 37;
    if (strcmp(name, "escape") == 0) return 38;
    if (strcmp(name, "tab") == 0)    return 39;
    if (strcmp(name, "shift") == 0)  return 40;
    if (strcmp(name, "ctrl") == 0)   return 41;
    if (strcmp(name, "alt") == 0)    return 42;
    if (strcmp(name, "up") == 0)     return 43;
    if (strcmp(name, "down") == 0)   return 44;
    if (strcmp(name, "left") == 0)   return 45;
    if (strcmp(name, "right") == 0)  return 46;
    if (strcmp(name, "backspace") == 0) return 47;
    if (strcmp(name, "delete") == 0) return 48;
    return -1;
}

/* ========================================================================= */
/* Keyboard API                                                              */
/* ========================================================================= */

/*
** engine.Input.isKeyDown(keyName) -> boolean
** Returns true if the key is currently held down.
*/
static int input_is_key_down(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    int idx = key_name_to_index(name);
    if (idx < 0 || idx >= MAX_KEYS) {
        lua_pushboolean(L, 0);
    } else {
        lua_pushboolean(L, input_state.keys[idx]);
    }
    return 1;
}

/*
** engine.Input.isKeyJustPressed(keyName) -> boolean
** Returns true only on the first frame the key is pressed.
*/
static int input_is_key_just_pressed(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    int idx = key_name_to_index(name);
    if (idx < 0 || idx >= MAX_KEYS) {
        lua_pushboolean(L, 0);
    } else {
        lua_pushboolean(L, input_state.keys[idx] && !input_state.keys_prev[idx]);
    }
    return 1;
}

/*
** engine.Input.isKeyJustReleased(keyName) -> boolean
** Returns true only on the first frame the key is released.
*/
static int input_is_key_just_released(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    int idx = key_name_to_index(name);
    if (idx < 0 || idx >= MAX_KEYS) {
        lua_pushboolean(L, 0);
    } else {
        lua_pushboolean(L, !input_state.keys[idx] && input_state.keys_prev[idx]);
    }
    return 1;
}

/* ========================================================================= */
/* Mouse API                                                                 */
/* ========================================================================= */

/*
** engine.Input.getMousePos() -> x, y
*/
static int input_get_mouse_pos(lua_State *L) {
    lua_pushnumber(L, input_state.mouse_x);
    lua_pushnumber(L, input_state.mouse_y);
    return 2;
}

/*
** engine.Input.isMouseDown([button]) -> boolean
** button: 1=left, 2=right, 3=middle (default: 1)
*/
static int input_is_mouse_down(lua_State *L) {
    int btn = (int)luaL_optinteger(L, 1, 1);
    if (btn < 1 || btn > MAX_MOUSE_BUTTONS) {
        lua_pushboolean(L, 0);
    } else {
        lua_pushboolean(L, input_state.mouse_buttons[btn - 1]);
    }
    return 1;
}

/*
** engine.Input.isMouseJustPressed([button]) -> boolean
*/
static int input_is_mouse_just_pressed(lua_State *L) {
    int btn = (int)luaL_optinteger(L, 1, 1);
    if (btn < 1 || btn > MAX_MOUSE_BUTTONS) {
        lua_pushboolean(L, 0);
    } else {
        lua_pushboolean(L, input_state.mouse_buttons[btn - 1] &&
                           !input_state.mouse_buttons_prev[btn - 1]);
    }
    return 1;
}

/*
** engine.Input.getScroll() -> x, y
** Returns scroll wheel delta since last frame.
*/
static int input_get_scroll(lua_State *L) {
    lua_pushnumber(L, input_state.scroll_x);
    lua_pushnumber(L, input_state.scroll_y);
    return 2;
}

/* ========================================================================= */
/* Frame Management                                                          */
/* ========================================================================= */

/*
** engine.Input.update()
** Called once per frame to latch previous-frame state (for justPressed/Released).
** Must be called at the END of the game loop.
*/
static int input_update(lua_State *L) {
    (void)L;
    memcpy(input_state.keys_prev, input_state.keys, sizeof(input_state.keys));
    memcpy(input_state.mouse_buttons_prev, input_state.mouse_buttons,
           sizeof(input_state.mouse_buttons));
    input_state.scroll_x = 0.0;
    input_state.scroll_y = 0.0;
    return 0;
}

/* ========================================================================= */
/* Registration                                                              */
/* ========================================================================= */

static const luaL_Reg input_funcs[] = {
    {"isKeyDown",          input_is_key_down},
    {"isKeyJustPressed",   input_is_key_just_pressed},
    {"isKeyJustReleased",  input_is_key_just_released},
    {"getMousePos",        input_get_mouse_pos},
    {"isMouseDown",        input_is_mouse_down},
    {"isMouseJustPressed", input_is_mouse_just_pressed},
    {"getScroll",          input_get_scroll},
    {"update",             input_update},
    {NULL, NULL}
};

void engine_register_input(lua_State *L) {
    /* Zero-initialize the input state */
    memset(&input_state, 0, sizeof(input_state));

    luaL_newlib(L, input_funcs);
    lua_setfield(L, -2, "Input");
}
