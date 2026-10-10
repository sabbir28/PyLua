/*
** PyLua Engine - Visual CPU / Bytecode Debugger PRO
** Modeled after x64dbg / OllyDbg CPU architecture.
** Provides live bytecode disassembly, jump arrows, execution pointer,
** register inspection, constants pool, breakpoints, source view, and console log.
*/

#define engine_debugger_c
#define LUA_CORE

#include "Engine_Debugger.h"
#include "Debugger/Debugger_Types.h"
#include "Debugger/Debugger_Core.h"
#include "Debugger/Debugger_Render.h"
#include "Debugger/Debugger_Disasm.h"
#include "Debugger/Debugger_UI.h"
#include "Debugger/Debugger_Input.h"

/* ========================================================================= */
/* Debugger Hook                                                             */
/* ========================================================================= */

static void debugger_hook(lua_State *L, lua_Debug *ar) {
    (void)ar;
    if (g_dbg.should_exit) return;

    CallInfo *ci = L->ci;
    if (!ci || !isLua(ci)) return;

    Proto *p = ci_func(ci)->p;
    if (!p) return;

    int cur_pc = pcRel(ci->u.l.savedpc, p);
    if (cur_pc < 0 || cur_pc >= p->sizecode) return;

    /* Update current proto if changed */
    if (p != g_dbg.current_proto) {
        decode_proto(p);
    }
    g_dbg.current_pc = cur_pc;
    g_dbg.selected_pc = cur_pc;

    /* Check if breakpoint hit */
    int bp_idx = find_breakpoint(cur_pc);
    if (bp_idx >= 0 && g_dbg.breakpoints[bp_idx].enabled) {
        g_dbg.status = DBG_STATUS_PAUSED;
        g_dbg.breakpoints[bp_idx].hit_count++;
        dbg_log_add(2, "[BP] Hit breakpoint at PC %04X (Line %d, hits: %d)",
                    cur_pc, luaG_getfuncline(p, cur_pc), g_dbg.breakpoints[bp_idx].hit_count);
    } else if (g_dbg.status == DBG_STATUS_STEP_INTO) {
        g_dbg.status = DBG_STATUS_PAUSED;
    } else if (g_dbg.status == DBG_STATUS_STEP_OVER) {
        int depth = get_call_depth(L);
        if (depth <= g_dbg.step_over_ci_depth) {
            g_dbg.status = DBG_STATUS_PAUSED;
        }
    }

    /* While running, maintain live UI refresh and check input */
    if (g_dbg.status == DBG_STATUS_RUNNING) {
        g_dbg.instructions_executed++;

        double now = glfwGetTime();
        if (g_dbg.execution_delay_ms > 0 || (now - g_dbg.last_ui_update >= 0.016)) {
            g_dbg.last_ui_update = now;
            glfwPollEvents();
            poll_global_hotkeys(L);
            render_debugger_gui(L);
            glfwSwapBuffers(g_dbg.window);

            if (g_dbg.execution_delay_ms > 0) {
#ifdef _WIN32
                Sleep((DWORD)g_dbg.execution_delay_ms);
#endif
            }
        } else if ((g_dbg.instructions_executed & 127) == 0) {
            glfwPollEvents();
            poll_global_hotkeys(L);
        }

        if (glfwWindowShouldClose(g_dbg.window) || g_dbg.should_exit) {
            g_dbg.should_exit = 1;
            luaL_error(L, "Debugger stopped by user.");
        }

        /* If paused by button click or hotkey, enter modal loop below! */
        if (g_dbg.status != DBG_STATUS_PAUSED) {
            return;
        }
    }

    /* Paused modal render loop */
    while (g_dbg.status == DBG_STATUS_PAUSED && !glfwWindowShouldClose(g_dbg.window)) {
        glfwPollEvents();
        poll_global_hotkeys(L);
        render_debugger_gui(L);
        glfwSwapBuffers(g_dbg.window);
        glfwWaitEventsTimeout(0.016); /* ~60 FPS */
    }

    if (glfwWindowShouldClose(g_dbg.window)) {
        g_dbg.should_exit = 1;
        luaL_error(L, "Debugger stopped by user.");
    }

    if (g_dbg.status == DBG_STATUS_STEP_OVER) {
        g_dbg.step_over_ci_depth = get_call_depth(L);
    }
}

/* ========================================================================= */
/* Entry Point: engine_launch_debugger                                       */
/* ========================================================================= */

int engine_launch_debugger(lua_State *L, const char *script_path, int argc, char **argv) {
    (void)argc; (void)argv;
    if (!script_path) {
        fprintf(stderr, "PyLua Debugger: No script file provided.\n");
        fprintf(stderr, "Usage: pylua --debug <script.lua> [arguments...]\n");
        return 0;
    }

    if (!glfwInit()) {
        fprintf(stderr, "PyLua Debugger: Failed to initialize GLFW.\n");
        return 0;
    }

    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
    GLFWwindow *window = glfwCreateWindow(1280, 768, "PyLua CPU / Bytecode Debugger PRO", NULL, NULL);
    if (!window) {
        fprintf(stderr, "PyLua Debugger: Failed to create GLFW window.\n");
        glfwTerminate();
        return 0;
    }

    /* Initialize debugger core state */
    dbg_state_init(window, script_path);

    glfwMakeContextCurrent(window);
    glfwShowWindow(window);
    glfwFocusWindow(window);
    glfwSwapInterval(1);

    glfwSetKeyCallback(window, dbg_key_callback);
    glfwSetMouseButtonCallback(window, dbg_mouse_button_callback);
    glfwSetCursorPosCallback(window, dbg_cursor_pos_callback);
    glfwSetScrollCallback(window, dbg_scroll_callback);
    glfwSetWindowSizeCallback(window, (GLFWwindowsizefun)dbg_window_size_callback);

    /* Load script source lines into memory for Source tab */
    load_script_source(script_path);

    /* Redirect print to debugger log */
    lua_register(L, "print", dbg_custom_print);

    dbg_log_add(2, "[INIT] PyLua CPU Debugger initialized successfully");
    dbg_log_add(2, "[FILE] Loaded script: %s (%d lines)", script_path, g_dbg.script_src.line_count);

    /* Compile script */
    int load_status = luaL_loadfilex(L, script_path, "bt");
    if (load_status != LUA_OK) {
        const char *err = lua_tostring(L, -1);
        dbg_log_add(1, "[COMPILE ERROR] %s", err ? err : "Unknown compilation error");
        g_dbg.status = DBG_STATUS_TERMINATED;
        while (!glfwWindowShouldClose(window)) {
            render_debugger_gui(L);
            glfwSwapBuffers(window);
            glfwWaitEventsTimeout(0.016);
        }
        dbg_state_cleanup();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 0;
    }

    /* Pre-decode main proto so disassembly shows up before first step */
    if (lua_isfunction(L, -1)) {
        LClosure *cl = clLvalue(s2v(L->top.p - 1));
        if (cl && cl->p) {
            decode_proto(cl->p);
            g_dbg.current_pc = 0;
            g_dbg.selected_pc = 0;
        }
    }

    /* Attach debug hook for instruction-level interception */
    lua_sethook(L, debugger_hook, LUA_MASKCOUNT | LUA_MASKCALL | LUA_MASKRET, 1);

    dbg_log_add(2, "[EXEC] Paused at entry point (PC: 0000). Press F5 to run, F7 to step.");

    /* Execute the script under the debugger */
    int run_status = lua_pcall(L, 0, LUA_MULTRET, 0);

    /* Script finished */
    g_dbg.status = DBG_STATUS_TERMINATED;
    if (run_status != LUA_OK) {
        const char *err = lua_tostring(L, -1);
        dbg_log_add(1, "[RUNTIME ERROR] %s", err ? err : "Execution failed");
    } else {
        dbg_log_add(2, "[EXEC] Script execution terminated normally (status: OK)");
    }

    /* Keep window open so developer can inspect final state, logs, and registers */
    while (!glfwWindowShouldClose(window)) {
        render_debugger_gui(L);
        glfwSwapBuffers(window);
        glfwWaitEventsTimeout(0.016);
    }

    dbg_state_cleanup();
    glfwDestroyWindow(window);
    glfwTerminate();
    return (run_status == LUA_OK) ? 1 : 0;
}
