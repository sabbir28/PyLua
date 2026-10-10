/*
** PyLua Visual Bytecode / CPU Debugger PRO
** Module: Debugger Input, GLFW Callbacks & Hotkeys
*/

#ifndef DEBUGGER_INPUT_H
#define DEBUGGER_INPUT_H

#include "Debugger_Types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* GLFW Callbacks */
void dbg_key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
void dbg_mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
void dbg_cursor_pos_callback(GLFWwindow *window, double xpos, double ypos);
void dbg_scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void dbg_window_size_callback(GLFWwindow *window, int width, int height);

/* Asynchronous Global Hotkeys Polling */
void poll_global_hotkeys(lua_State *L);

#ifdef __cplusplus
}
#endif

#endif /* DEBUGGER_INPUT_H */
