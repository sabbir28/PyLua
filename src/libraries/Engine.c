#define engine_c
#define LUA_LIB

#include "../include/lua.h"
#include "../include/lauxlib.h"
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#include "Engine/Window/Engine_Window.h"
#include "Engine/Physics/Engine_Physics.h"
#include "Engine/Graphics/Engine_Graphics.h"
#include "Engine/Input/Engine_Input.h"
#include "Engine/Audio/Engine_Audio.h"
#include "Engine/Assets/Engine_Assets.h"
#include "Engine/UI/Engine_UI.h"
#include "Engine/Animation/Engine_Animation.h"

/* ========================================================================= */
/* Types & Constants                                                         */
/* ========================================================================= */

#define ENGINE_ENTITY_MT "Engine.Entity"
#define MAX_ENTITIES 1024

typedef struct {
    char name[64];
    int active;
    int script_ref; /* Reference to a table of scripts in the Lua registry */
} Entity;

typedef struct {
    lua_State *L_main;
    Entity *entities[MAX_ENTITIES];
    int entity_count;
} Scene;

static Scene g_scene;

/* ========================================================================= */
/* Worker Threading (Concurrency)                                             */
/* ========================================================================= */

typedef struct {
    char script_path[256];
    int running;
#ifdef _WIN32
    HANDLE thread;
#else
    pthread_t thread;
#endif
} Worker;

#ifdef _WIN32
static DWORD WINAPI worker_thread_func(LPVOID lpParam) {
#else
static void* worker_thread_func(void* lpParam) {
#endif
    Worker *worker = (Worker *)lpParam;
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    
    /* In a real engine, we'd also register a minimal 'engine' API here for workers */
    
    if (luaL_dofile(L, worker->script_path) != LUA_OK) {
        fprintf(stderr, "[Worker] Error: %s\n", lua_tostring(L, -1));
    }
    
    lua_close(L);
    worker->running = 0;
    return 0;
}

static int engine_create_worker(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    Worker *worker = (Worker *)malloc(sizeof(Worker));
    if (!worker) return luaL_error(L, "Out of memory");
    strncpy(worker->script_path, path, 255);
    worker->running = 1;
    
#ifdef _WIN32
    worker->thread = CreateThread(NULL, 0, worker_thread_func, worker, 0, NULL);
#else
    pthread_create(&worker->thread, NULL, worker_thread_func, worker);
#endif

    lua_pushlightuserdata(L, worker);
    return 1;
}

static int engine_cleanup_worker(lua_State *L) {
    Worker *worker = (Worker *)lua_touserdata(L, 1);
    if (worker) {
        if (worker->running) {
            /* In a professional engine, we would signal the worker to stop gracefully. 
               For now, we just ensure it's handled. */
        }
#ifdef _WIN32
        CloseHandle(worker->thread);
#else
        pthread_detach(worker->thread);
#endif
        free(worker);
    }
    return 0;
}

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
/* Entity System                                                             */
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

static Entity *check_entity(lua_State *L, int idx) {
    return (Entity *)luaL_checkudata(L, idx, ENGINE_ENTITY_MT);
}

static int entity_add_script(lua_State *L) {
    Entity *ent = check_entity(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);

    /* Store the script in the entity's script table */
    lua_rawgeti(L, LUA_REGISTRYINDEX, ent->script_ref);
    lua_pushvalue(L, 2);
    luaL_ref(L, -2);
    lua_pop(L, 1);
    return 0;
}

static int entity_destroy(lua_State *L) {
    Entity *ent = check_entity(L, 1);
    ent->active = 0;
    luaL_unref(L, LUA_REGISTRYINDEX, ent->script_ref);
    ent->script_ref = LUA_NOREF;
    return 0;
}

static int scene_create_entity(lua_State *L) {
    const char *name = luaL_optstring(L, 1, "Entity");
    if (g_scene.entity_count >= MAX_ENTITIES) {
        /* Try to find an inactive slot first */
        for (int i = 0; i < g_scene.entity_count; i++) {
            if (!g_scene.entities[i]->active) {
                Entity *ent = g_scene.entities[i];
                strncpy(ent->name, name, 63);
                ent->active = 1;
                lua_newtable(L);
                ent->script_ref = luaL_ref(L, LUA_REGISTRYINDEX);
                
                /* Return the existing userdata but update its metatable/state */
                /* Actually, it's safer to just create a new one if possible or manage slots better. 
                   For now, we'll just push the existing one. */
                return 1; 
            }
        }
        return luaL_error(L, "Too many active entities");
    }

    Entity *ent = (Entity *)lua_newuserdatauv(L, sizeof(Entity), 0);
    strncpy(ent->name, name, 63);
    ent->active = 1;
    
    lua_newtable(L);
    ent->script_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    luaL_getmetatable(L, ENGINE_ENTITY_MT);
    lua_setmetatable(L, -2);

    g_scene.entities[g_scene.entity_count++] = ent;
    return 1;
}

/* ========================================================================= */
/* Game Loop & Engine State                                                 */
/* ========================================================================= */

static struct {
    int running;
    double last_frame_time;
    double delta_time;
    int current_screen_ref; /* Reference to the active Screen table in the registry */
} engine_state;

/* ========================================================================= */
/* Screen System                                                            */
/* ========================================================================= */

static int engine_screen_set(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    /* 1. Call onExit of the old screen if it exists */
    if (engine_state.current_screen_ref != LUA_NOREF) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, engine_state.current_screen_ref);
        lua_getfield(L, -1, "onExit");
        if (lua_isfunction(L, -1)) {
            lua_pushvalue(L, -2); /* self */
            lua_pcall(L, 1, 0, 0);
        } else lua_pop(L, 1);
        luaL_unref(L, LUA_REGISTRYINDEX, engine_state.current_screen_ref);
        lua_pop(L, 1);
    }

    /* 2. Set the new screen */
    lua_pushvalue(L, 1);
    engine_state.current_screen_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    /* 3. Call onEnter of the new screen */
    lua_getfield(L, 1, "onEnter");
    if (lua_isfunction(L, -1)) {
        lua_pushvalue(L, 1); /* self */
        lua_pcall(L, 1, 0, 0);
    } else lua_pop(L, 1);

    return 0;
}

static int engine_screen_get(lua_State *L) {
    if (engine_state.current_screen_ref == LUA_NOREF) lua_pushnil(L);
    else lua_rawgeti(L, LUA_REGISTRYINDEX, engine_state.current_screen_ref);
    return 1;
}

static int engine_run(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    lua_getfield(L, 1, "init");
    int has_init = lua_isfunction(L, -1);
    lua_getfield(L, 1, "update");
    int has_update = lua_isfunction(L, -1);
    lua_getfield(L, 1, "draw");
    int has_draw = lua_isfunction(L, -1);
    lua_getfield(L, 1, "window");
    if (lua_isnil(L, -1)) return luaL_error(L, "engine.run: 'window' field is required");
    lua_getfield(L, 1, "physicsWorld");
    int has_physics = !lua_isnil(L, -1);

    if (has_init) {
        lua_pushvalue(L, 2);
        if (lua_pcall(L, 0, 0, 0) != LUA_OK) return luaL_error(L, "Init Error: %s", lua_tostring(L, -1));
    }

    engine_state.running = 1;
    engine_state.last_frame_time = glfwGetTime();

    while (engine_state.running) {
        double current_time = glfwGetTime();
        engine_state.delta_time = current_time - engine_state.last_frame_time;
        engine_state.last_frame_time = current_time;

        glfwPollEvents();
        GLFWwindow* current_win = glfwGetCurrentContext();
        if (!current_win || glfwWindowShouldClose(current_win)) break;

        /* PHYSICS STEP */
        if (has_physics) {
            lua_pushvalue(L, 6);
            lua_getfield(L, -1, "step");
            lua_pushvalue(L, -2);
            lua_pushnumber(L, engine_state.delta_time);
            lua_pcall(L, 2, 0, 0);
            lua_pop(L, 1);
        }

        /* ENTITY UPDATES (The Ecosystem!) */
        for (int i = 0; i < g_scene.entity_count; i++) {
            Entity *ent = g_scene.entities[i];
            if (!ent->active) continue;
            
            lua_rawgeti(L, LUA_REGISTRYINDEX, ent->script_ref);
            lua_pushnil(L);
            while (lua_next(L, -2) != 0) {
                /* 'key' at -2, 'script' at -1 */
                lua_getfield(L, -1, "update");
                if (lua_isfunction(L, -1)) {
                    lua_pushvalue(L, -2); /* self */
                    lua_pushnumber(L, engine_state.delta_time);
                    lua_pcall(L, 2, 0, 0);
                } else lua_pop(L, 1);
                lua_pop(L, 1); /* pop value, keep key */
            }
            lua_pop(L, 1); /* pop script table */
        }

        if (has_update) {
            lua_pushvalue(L, 3);
            lua_pushnumber(L, engine_state.delta_time);
            lua_pcall(L, 1, 0, 0);
        }

        if (has_draw) {
            lua_pushvalue(L, 4);
            lua_pcall(L, 0, 0, 0);
        }

        /* Latch Input State (for justPressed/Released) */
        lua_getglobal(L, "engine");
        lua_getfield(L, -1, "Input");
        lua_getfield(L, -1, "update");
        lua_pcall(L, 0, 0, 0);
        lua_pop(L, 2); /* pop engine, Input */

        glfwSwapBuffers(current_win);
    }

    return 0;
}

static int engine_get_dt(lua_State *L) {
    lua_pushnumber(L, engine_state.delta_time);
    return 1;
}

static const luaL_Reg entity_methods[] = {
    {"addScript", entity_add_script},
    {"destroy",   entity_destroy},
    {NULL, NULL}
};

static const luaL_Reg scene_funcs[] = {
    {"createEntity", scene_create_entity},
    {NULL, NULL}
};

static const luaL_Reg engine_funcs[] = {
    {"run",           engine_run},
    {"createWorker",  engine_create_worker},
    {"destroyWorker", engine_cleanup_worker},
    {"getDeltaTime",  engine_get_dt},
    {"stop",          engine_stop},
    {NULL, NULL}
};

static const luaL_Reg screen_funcs[] = {
    {"set", engine_screen_set},
    {"get", engine_screen_get},
    {NULL, NULL}
};

LUAMOD_API int luaopen_engine(lua_State *L) {
    g_scene.L_main = L;
    g_scene.entity_count = 0;
    engine_state.current_screen_ref = LUA_NOREF;

    /* Entity Metatable */
    luaL_newmetatable(L, ENGINE_ENTITY_MT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, entity_methods, 0);
    lua_pop(L, 1);

    luaL_newlib(L, engine_funcs);

    /* Scene */
    luaL_newlib(L, scene_funcs);
    lua_setfield(L, -2, "Scene");

    /* Screen */
    luaL_newlib(L, screen_funcs);
    lua_setfield(L, -2, "Screen");

    engine_register_window(L);
    engine_register_physics(L);
    engine_register_graphics(L);
    engine_register_input(L);
    engine_register_audio(L);
    engine_register_assets(L);
    engine_register_ui(L);
    engine_register_animation(L);

    return 1;
}
