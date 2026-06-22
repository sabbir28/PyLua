/*
** PyLua Engine - Window Subsystem
** Provides window creation, management, and event polling for the 2D game engine.
*/

#define engine_window_c
#define LUA_LIB

#include "../../../../include/lprefix.h"

#include <stdio.h>
#include <string.h>

#include <GLFW/glfw3.h>

#include "../../../../include/lua.h"
#include "../../../../include/lauxlib.h"
#include "Engine_Window.h"

/* ========================================================================= */
/* Window State                                                              */
/* ========================================================================= */

#define ENGINE_WINDOW_MT "Engine.Window"

typedef struct {
    GLFWwindow *handle;
    int width;
    int height;
    const char *title;
    int is_open;
    int fullscreen;
    double fps_target;
} EngineWindow;

static EngineWindow *check_window(lua_State *L, int idx) {
    return (EngineWindow *)luaL_checkudata(L, idx, ENGINE_WINDOW_MT);
}

/* ========================================================================= */
/* API Functions                                                             */
/* ========================================================================= */

/*
** engine.Window.getMonitors() -> table
** Returns a list of available monitors.
*/
static int win_get_monitors(lua_State *L) {
    int count;
    GLFWmonitor **monitors = glfwGetMonitors(&count);
    lua_newtable(L);
    for (int i = 0; i < count; i++) {
        const char *name = glfwGetMonitorName(monitors[i]);
        const GLFWvidmode *mode = glfwGetVideoMode(monitors[i]);
        int x, y;
        glfwGetMonitorPos(monitors[i], &x, &y);

        lua_newtable(L);
        lua_pushstring(L, name);
        lua_setfield(L, -2, "name");
        lua_pushinteger(L, mode->width);
        lua_setfield(L, -2, "width");
        lua_pushinteger(L, mode->height);
        lua_setfield(L, -2, "height");
        lua_pushinteger(L, x);
        lua_setfield(L, -2, "x");
        lua_pushinteger(L, y);
        lua_setfield(L, -2, "y");
        
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

/*
** engine.Window.create(title, width, height [, fullscreen, monitorIdx])
** Creates and returns a new game window.
*/
static int win_create(lua_State *L) {
    const char *title = luaL_checkstring(L, 1);
    int w = (int)luaL_optinteger(L, 2, 800);
    int h = (int)luaL_optinteger(L, 3, 600);
    int fs = lua_toboolean(L, 4);
    int monitorIdx = (int)luaL_optinteger(L, 5, 1) - 1;

    if (!glfwInit()) {
        return luaL_error(L, "Could not initialize GLFW");
    }

    /* Set window hints */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWmonitor *monitor = NULL;
    if (fs) {
        int count;
        GLFWmonitor **monitors = glfwGetMonitors(&count);
        if (monitorIdx >= 0 && monitorIdx < count) {
            monitor = monitors[monitorIdx];
        } else {
            monitor = glfwGetPrimaryMonitor();
        }
    }

    GLFWwindow *handle = glfwCreateWindow(w, h, title, monitor, NULL);
    if (!handle) {
        glfwTerminate();
        return luaL_error(L, "Could not create GLFW window");
    }

    glfwMakeContextCurrent(handle);
    glfwSwapInterval(1); /* Enable vsync */

    EngineWindow *win = (EngineWindow *)lua_newuserdatauv(L, sizeof(EngineWindow), 0);
    win->handle = handle;
    win->width = w;
    win->height = h;
    win->title = title;
    win->is_open = 1;
    win->fullscreen = fs;
    win->fps_target = 60.0;

    luaL_getmetatable(L, ENGINE_WINDOW_MT);
    lua_setmetatable(L, -2);

    (void)fprintf(stdout, "[Engine.Window] Created '%s' (%dx%d)%s on monitor %d\n",
                  title, w, h, fs ? " fullscreen" : "", monitorIdx + 1);
    return 1;
}

/*
** window:close()
** Closes the window.
*/
static int win_close(lua_State *L) {
    EngineWindow *win = check_window(L, 1);
    if (win->handle) {
        glfwSetWindowShouldClose(win->handle, GLFW_TRUE);
        win->is_open = 0;
    }
    return 0;
}

/*
** window:isOpen() -> boolean
** Returns true if the window is still open.
*/
static int win_is_open(lua_State *L) {
    EngineWindow *win = check_window(L, 1);
    if (win->handle) {
        win->is_open = !glfwWindowShouldClose(win->handle);
    }
    lua_pushboolean(L, win->is_open);
    return 1;
}

/*
** window:pollEvents() -> boolean
** Processes pending events. Returns true if the window remains open.
*/
static int win_poll_events(lua_State *L) {
    EngineWindow *win = check_window(L, 1);
    glfwPollEvents();
    if (win->handle) {
        win->is_open = !glfwWindowShouldClose(win->handle);
        glfwSwapBuffers(win->handle);
    }
    lua_pushboolean(L, win->is_open);
    return 1;
}

/*
** window:getSize() -> width, height
** Returns the current window dimensions.
*/
static int win_get_size(lua_State *L) {
    EngineWindow *win = check_window(L, 1);
    if (win->handle) {
        glfwGetWindowSize(win->handle, &win->width, &win->height);
    }
    lua_pushinteger(L, win->width);
    lua_pushinteger(L, win->height);
    return 2;
}

/*
** window:setSize(width, height)
** Resizes the window.
*/
static int win_set_size(lua_State *L) {
    EngineWindow *win = check_window(L, 1);
    win->width = (int)luaL_checkinteger(L, 2);
    win->height = (int)luaL_checkinteger(L, 3);
    if (win->handle) {
        glfwSetWindowSize(win->handle, win->width, win->height);
    }
    return 0;
}

/*
** window:setTitle(title)
** Changes the window title.
*/
static int win_set_title(lua_State *L) {
    EngineWindow *win = check_window(L, 1);
    win->title = luaL_checkstring(L, 2);
    if (win->handle) {
        glfwSetWindowTitle(win->handle, win->title);
    }
    return 0;
}

/*
** window:setFPS(target)
** Sets the target frames per second.
*/
static int win_set_fps(lua_State *L) {
    EngineWindow *win = check_window(L, 1);
    win->fps_target = luaL_checknumber(L, 2);
    /* For now, just a placeholder. In a professional engine, this would control timing loop */
    return 0;
}

/*
** window:__gc
** Cleanup window on garbage collection.
*/
static int win_gc(lua_State *L) {
    EngineWindow *win = check_window(L, 1);
    if (win->handle) {
        glfwDestroyWindow(win->handle);
        win->handle = NULL;
    }
    return 0;
}

/*
** window:__tostring
*/
static int win_tostring(lua_State *L) {
    EngineWindow *win = check_window(L, 1);
    lua_pushfstring(L, "Window('%s', %dx%d)", win->title, win->width, win->height);
    return 1;
}

/* Module-level functions */
static const luaL_Reg window_funcs[] = {
    {"create", win_create},
    {NULL, NULL}
};

/* Window instance methods */
static const luaL_Reg window_methods[] = {
    {"close",      win_close},
    {"isOpen",     win_is_open},
    {"pollEvents", win_poll_events},
    {"getSize",    win_get_size},
    {"setSize",    win_set_size},
    {"setTitle",   win_set_title},
    {"setFPS",     win_set_fps},
    {"__tostring", win_tostring},
    {"__gc",       win_gc},
    {NULL, NULL}
};

/* ========================================================================= */
/* Registration                                                              */
/* ========================================================================= */

void engine_register_window(lua_State *L) {
    /* Create metatable for Window userdata */
    luaL_newmetatable(L, ENGINE_WINDOW_MT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, window_methods, 0);
    lua_pop(L, 1);

    /* Create the Window sub-table on the engine table (stack top) */
    luaL_newlib(L, window_funcs);
    lua_setfield(L, -2, "Window");
}
