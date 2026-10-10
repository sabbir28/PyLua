/*
** PyLua Visual Bytecode / CPU Debugger PRO
** Module: Debugger Input, GLFW Callbacks & Hotkeys Implementation
*/

#include "Debugger_Input.h"
#include "Debugger_Core.h"

/* ========================================================================= */
/* GLFW Event Callbacks                                                      */
/* ========================================================================= */

void dbg_key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    (void)window; (void)scancode; (void)mods;
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;

    /* For non-navigation keys, only act on PRESS (not repeat) */
    if (action == GLFW_REPEAT && key != GLFW_KEY_UP && key != GLFW_KEY_DOWN &&
        key != GLFW_KEY_PAGE_UP && key != GLFW_KEY_PAGE_DOWN) {
        return;
    }

    switch (key) {
        case GLFW_KEY_F5:
        case GLFW_KEY_SPACE:
            if (g_dbg.status == DBG_STATUS_RUNNING) {
                g_dbg.status = DBG_STATUS_PAUSED;
                dbg_log_add(2, "[EXEC] Paused by user (F5/Space)");
            } else {
                g_dbg.status = DBG_STATUS_RUNNING;
                dbg_log_add(2, "[EXEC] Continued execution (F5/Space)");
            }
            break;
        case GLFW_KEY_P:
        case GLFW_KEY_PAUSE:
        case GLFW_KEY_F12:
            g_dbg.status = DBG_STATUS_PAUSED;
            dbg_log_add(2, "[EXEC] Execution paused (P/Pause)");
            break;
        case GLFW_KEY_F7:
        case GLFW_KEY_F11:
            g_dbg.status = DBG_STATUS_STEP_INTO;
            break;
        case GLFW_KEY_F8:
        case GLFW_KEY_F10:
            g_dbg.step_over_ci_depth = 0; /* will be populated in hook */
            g_dbg.status = DBG_STATUS_STEP_OVER;
            break;
        case GLFW_KEY_F2:
        case GLFW_KEY_F9:
            if (g_dbg.selected_pc >= 0 && g_dbg.current_proto) {
                int line = luaG_getfuncline(g_dbg.current_proto, g_dbg.selected_pc);
                toggle_breakpoint(g_dbg.selected_pc, line);
            }
            break;
        case GLFW_KEY_F6:
            if (g_dbg.execution_delay_ms == 0) g_dbg.execution_delay_ms = 10;
            else if (g_dbg.execution_delay_ms == 10) g_dbg.execution_delay_ms = 50;
            else if (g_dbg.execution_delay_ms == 50) g_dbg.execution_delay_ms = 100;
            else g_dbg.execution_delay_ms = 0;
            dbg_log_add(2, "[SPEED] Execution speed set to %d ms/inst (F6)", g_dbg.execution_delay_ms);
            break;
        case GLFW_KEY_1: g_dbg.active_tab = TAB_CPU; break;
        case GLFW_KEY_2: g_dbg.active_tab = TAB_LOG; break;
        case GLFW_KEY_3: g_dbg.active_tab = TAB_BREAKPOINTS; break;
        case GLFW_KEY_4: g_dbg.active_tab = TAB_MEMORY; break;
        case GLFW_KEY_5: g_dbg.active_tab = TAB_SCRIPT; break;
        case GLFW_KEY_UP:
            if (g_dbg.selected_pc > 0) {
                g_dbg.selected_pc--;
                if (g_dbg.selected_pc < g_dbg.disasm_scroll_pc)
                    g_dbg.disasm_scroll_pc = g_dbg.selected_pc;
            }
            break;
        case GLFW_KEY_DOWN:
            if (g_dbg.selected_pc < g_dbg.disasm_count - 1) {
                g_dbg.selected_pc++;
                int max_vis = (int)(((float)g_dbg.win_h - 84.0f - 24.0f - 22.0f) / 22.0f);
                if (max_vis < 5) max_vis = 5;
                if (g_dbg.selected_pc >= g_dbg.disasm_scroll_pc + max_vis)
                    g_dbg.disasm_scroll_pc = g_dbg.selected_pc - max_vis + 1;
            }
            break;
        case GLFW_KEY_PAGE_UP:
            g_dbg.disasm_scroll_pc = (g_dbg.disasm_scroll_pc > 10) ? g_dbg.disasm_scroll_pc - 10 : 0;
            break;
        case GLFW_KEY_PAGE_DOWN:
            g_dbg.disasm_scroll_pc += 10;
            break;
    }
}

void dbg_mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
    (void)window; (void)mods;
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            g_dbg.mouse_left_down = 1;
            g_dbg.mouse_left_pressed = 1;
        } else if (action == GLFW_RELEASE) {
            g_dbg.mouse_left_down = 0;
            g_dbg.disasm_sb_dragging = 0;
            g_dbg.reg_sb_dragging = 0;
            g_dbg.log_sb_dragging = 0;
            g_dbg.src_sb_dragging = 0;
        }
    }
}

void dbg_cursor_pos_callback(GLFWwindow *window, double xpos, double ypos) {
    (void)window;
    g_dbg.mouse_x = xpos;
    g_dbg.mouse_y = ypos;
}

void dbg_scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    (void)window; (void)xoffset;
    if (g_dbg.active_tab == TAB_CPU) {
        if (g_dbg.mouse_x < (double)g_dbg.win_w - 440.0) {
            g_dbg.disasm_scroll_pc -= (int)(yoffset * 3.0);
            if (g_dbg.disasm_scroll_pc < 0) g_dbg.disasm_scroll_pc = 0;
        } else {
            g_dbg.reg_scroll -= (int)yoffset;
            if (g_dbg.reg_scroll < 0) g_dbg.reg_scroll = 0;
        }
    } else if (g_dbg.active_tab == TAB_LOG) {
        g_dbg.log_scroll -= (int)(yoffset * 2.0);
        if (g_dbg.log_scroll < 0) g_dbg.log_scroll = 0;
    } else if (g_dbg.active_tab == TAB_SCRIPT) {
        g_dbg.src_scroll -= (int)(yoffset * 2.0);
        if (g_dbg.src_scroll < 0) g_dbg.src_scroll = 0;
    }
}

void dbg_window_size_callback(GLFWwindow *window, int width, int height) {
    (void)window;
    g_dbg.win_w = width;
    g_dbg.win_h = height;
}

/* ========================================================================= */
/* Global Asynchronous Hotkeys Polling                                       */
/* ========================================================================= */

#ifdef _WIN32
static HotkeyState g_hotkeys = {0};

static int is_key_just_pressed(int vk, int *key_state) {
    int down = (GetAsyncKeyState(vk) & 0x8000) != 0;
    if (down && !(*key_state)) {
        *key_state = 1;
        return 1;
    }
    if (!down) {
        *key_state = 0;
    }
    return 0;
}

void poll_global_hotkeys(lua_State *L) {
    /* If the GLFW debugger window is focused, GLFW's dbg_key_callback handles events.
       Only poll global hotkeys when another window (such as the terminal console) is focused. */
    if (g_dbg.window && glfwGetWindowAttrib(g_dbg.window, GLFW_FOCUSED)) {
        return;
    }

    /* Speed toggle (F6) */
    if (is_key_just_pressed(VK_F6, &g_hotkeys.f6_down)) {
        if (g_dbg.execution_delay_ms == 0) g_dbg.execution_delay_ms = 10;
        else if (g_dbg.execution_delay_ms == 10) g_dbg.execution_delay_ms = 50;
        else if (g_dbg.execution_delay_ms == 50) g_dbg.execution_delay_ms = 100;
        else g_dbg.execution_delay_ms = 0;
        dbg_log_add(2, "[SPEED] Execution speed set to %d ms/inst (F6)", g_dbg.execution_delay_ms);
    }

    /* Breakpoint toggle (F2 / F9) */
    if (is_key_just_pressed(VK_F2, &g_hotkeys.f2_down) ||
        is_key_just_pressed(VK_F9, &g_hotkeys.f9_down)) {
        if (g_dbg.selected_pc >= 0 && g_dbg.current_proto) {
            int line = luaG_getfuncline(g_dbg.current_proto, g_dbg.selected_pc);
            toggle_breakpoint(g_dbg.selected_pc, line);
        }
    }

    if (g_dbg.status == DBG_STATUS_RUNNING) {
        /* Play / Pause toggle while running (F5, Space, Pause, P, F12) */
        if (is_key_just_pressed(VK_F5, &g_hotkeys.f5_down) ||
            is_key_just_pressed(VK_SPACE, &g_hotkeys.space_down) ||
            is_key_just_pressed(VK_PAUSE, &g_hotkeys.pause_down) ||
            is_key_just_pressed('P', &g_hotkeys.p_down) ||
            is_key_just_pressed(VK_F12, &g_hotkeys.f12_down)) {
            g_dbg.status = DBG_STATUS_PAUSED;
            dbg_log_add(2, "[EXEC] Paused execution (F5/Space/P)");
        }
    } else if (g_dbg.status == DBG_STATUS_PAUSED) {
        /* Continue (F5, Space) */
        if (is_key_just_pressed(VK_F5, &g_hotkeys.f5_down) ||
            is_key_just_pressed(VK_SPACE, &g_hotkeys.space_down)) {
            g_dbg.status = DBG_STATUS_RUNNING;
            dbg_log_add(2, "[EXEC] Continued execution (F5/Space)");
        }
        /* Step Into (F7, F11) */
        else if (is_key_just_pressed(VK_F7, &g_hotkeys.f7_down) ||
                 is_key_just_pressed(VK_F11, &g_hotkeys.f11_down)) {
            g_dbg.status = DBG_STATUS_STEP_INTO;
        }
        /* Step Over (F8, F10) */
        else if (is_key_just_pressed(VK_F8, &g_hotkeys.f8_down) ||
                 is_key_just_pressed(VK_F10, &g_hotkeys.f10_down)) {
            g_dbg.step_over_ci_depth = get_call_depth(L);
            g_dbg.status = DBG_STATUS_STEP_OVER;
        }

        /* Tabs */
        if (is_key_just_pressed('1', &g_hotkeys.k1_down)) g_dbg.active_tab = TAB_CPU;
        if (is_key_just_pressed('2', &g_hotkeys.k2_down)) g_dbg.active_tab = TAB_LOG;
        if (is_key_just_pressed('3', &g_hotkeys.k3_down)) g_dbg.active_tab = TAB_BREAKPOINTS;
        if (is_key_just_pressed('4', &g_hotkeys.k4_down)) g_dbg.active_tab = TAB_MEMORY;
        if (is_key_just_pressed('5', &g_hotkeys.k5_down)) g_dbg.active_tab = TAB_SCRIPT;

        /* Up / Down navigation */
        if (is_key_just_pressed(VK_UP, &g_hotkeys.up_down)) {
            if (g_dbg.selected_pc > 0) {
                g_dbg.selected_pc--;
                if (g_dbg.selected_pc < g_dbg.disasm_scroll_pc)
                    g_dbg.disasm_scroll_pc = g_dbg.selected_pc;
            }
        }
        if (is_key_just_pressed(VK_DOWN, &g_hotkeys.down_down)) {
            if (g_dbg.selected_pc < g_dbg.disasm_count - 1) {
                g_dbg.selected_pc++;
                int max_vis = (int)(((float)g_dbg.win_h - 84.0f - 24.0f - 22.0f) / 22.0f);
                if (max_vis < 5) max_vis = 5;
                if (g_dbg.selected_pc >= g_dbg.disasm_scroll_pc + max_vis)
                    g_dbg.disasm_scroll_pc = g_dbg.selected_pc - max_vis + 1;
            }
        }
        if (is_key_just_pressed(VK_PRIOR, &g_hotkeys.pgup_down)) {
            g_dbg.disasm_scroll_pc = (g_dbg.disasm_scroll_pc > 10) ? g_dbg.disasm_scroll_pc - 10 : 0;
        }
        if (is_key_just_pressed(VK_NEXT, &g_hotkeys.pgdn_down)) {
            g_dbg.disasm_scroll_pc += 10;
        }
    }
}
#else
void poll_global_hotkeys(lua_State *L) {
    (void)L;
}
#endif

