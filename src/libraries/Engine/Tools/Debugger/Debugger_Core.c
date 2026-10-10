/*
** PyLua Visual Bytecode / CPU Debugger PRO
** Module: Debugger Core & State Management Implementation
*/

#include "Debugger_Core.h"
#include <stdarg.h>

/* Global Debugger State Instance */
DebuggerState g_dbg = { 0 };

void dbg_state_init(GLFWwindow *window, const char *script_path) {
    g_dbg.window = window;
    g_dbg.win_w = 1280;
    g_dbg.win_h = 768;
    g_dbg.status = DBG_STATUS_PAUSED;
    g_dbg.active_tab = TAB_CPU;
    g_dbg.should_exit = 0;
    g_dbg.instructions_executed = 0;
    g_dbg.start_time = glfwGetTime();
    g_dbg.last_ui_update = glfwGetTime();
    g_dbg.execution_delay_ms = 0;
    g_dbg.bp_count = 0;
    g_dbg.log_start = 0;
    g_dbg.log_count = 0;
    g_dbg.log_scroll = 0;
    g_dbg.reg_scroll = 0;
    g_dbg.k_scroll = 0;
    g_dbg.src_scroll = 0;
    g_dbg.disasm_lines = NULL;
    g_dbg.disasm_count = 0;
    g_dbg.current_proto = NULL;
    g_dbg.current_pc = 0;
    g_dbg.selected_pc = 0;
    g_dbg.disasm_scroll_pc = 0;
    g_dbg.step_over_ci_depth = 0;
    g_dbg.disasm_sb_dragging = 0;
    g_dbg.disasm_sb_drag_offset_y = 0.0f;
    g_dbg.reg_sb_dragging = 0;
    g_dbg.reg_sb_drag_offset_y = 0.0f;
    g_dbg.k_sb_dragging = 0;
    g_dbg.k_sb_drag_offset_y = 0.0f;
    g_dbg.log_sb_dragging = 0;
    g_dbg.log_sb_drag_offset_y = 0.0f;
    g_dbg.src_sb_dragging = 0;
    g_dbg.src_sb_drag_offset_y = 0.0f;
    g_dbg.last_src_active_line = 0;
    if (script_path) {
        strncpy(g_dbg.script_path, script_path, sizeof(g_dbg.script_path) - 1);
        g_dbg.script_path[sizeof(g_dbg.script_path) - 1] = '\0';
    } else {
        g_dbg.script_path[0] = '\0';
    }
}

void dbg_state_cleanup(void) {
    free_script_source();
    if (g_dbg.disasm_lines) {
        free(g_dbg.disasm_lines);
        g_dbg.disasm_lines = NULL;
        g_dbg.disasm_count = 0;
    }
}

/* ========================================================================= */
/* Logging Helpers (Circular Ring Buffer)                                    */
/* ========================================================================= */

void dbg_log_add(int level, const char *fmt, ...) {
    int idx;
    if (g_dbg.log_count < MAX_LOG_LINES) {
        idx = (g_dbg.log_start + g_dbg.log_count) % MAX_LOG_LINES;
        g_dbg.log_count++;
    } else {
        idx = g_dbg.log_start;
        g_dbg.log_start = (g_dbg.log_start + 1) % MAX_LOG_LINES;
    }
    LogEntry *entry = &g_dbg.logs[idx];
    entry->timestamp = glfwGetTime();
    entry->level = level;
    va_list args;
    va_start(args, fmt);
    vsnprintf(entry->text, sizeof(entry->text), fmt, args);
    va_end(args);
    /* Auto-scroll log to bottom */
    if (g_dbg.log_count > 25) g_dbg.log_scroll = g_dbg.log_count - 25;
}

int dbg_custom_print(lua_State *L) {
    int n = lua_gettop(L);
    char buf[1024];
    int offset = 0;
    buf[0] = '\0';
    for (int i = 1; i <= n; i++) {
        size_t len = 0;
        const char *s = luaL_tolstring(L, i, &len);
        if (i > 1 && offset < (int)sizeof(buf) - 2) {
            buf[offset++] = '\t';
        }
        if (offset + (int)len < (int)sizeof(buf) - 2) {
            memcpy(buf + offset, s, len);
            offset += (int)len;
            buf[offset] = '\0';
        }
        lua_pop(L, 1);
    }
    printf("%s\n", buf);
    fflush(stdout);
    dbg_log_add(0, "%s", buf);
    return 0;
}

/* ========================================================================= */
/* Source Code Loader                                                        */
/* ========================================================================= */

void free_script_source(void) {
    for (int i = 0; i < g_dbg.script_src.line_count; i++) {
        if (g_dbg.script_src.lines[i]) {
            free(g_dbg.script_src.lines[i]);
            g_dbg.script_src.lines[i] = NULL;
        }
    }
    g_dbg.script_src.line_count = 0;
}

void load_script_source(const char *path) {
    free_script_source();
    if (!path) return;
    strncpy(g_dbg.script_src.file_path, path, sizeof(g_dbg.script_src.file_path) - 1);
    g_dbg.script_src.file_path[sizeof(g_dbg.script_src.file_path) - 1] = '\0';

    FILE *f = fopen(path, "r");
    if (!f) return;
    char line_buf[512];
    while (fgets(line_buf, sizeof(line_buf), f) && g_dbg.script_src.line_count < MAX_SCRIPT_LINES) {
        size_t len = strlen(line_buf);
        if (len > 0 && (line_buf[len - 1] == '\n' || line_buf[len - 1] == '\r')) {
            line_buf[--len] = '\0';
        }
        if (len > 0 && (line_buf[len - 1] == '\n' || line_buf[len - 1] == '\r')) {
            line_buf[--len] = '\0';
        }
#ifdef _WIN32
        g_dbg.script_src.lines[g_dbg.script_src.line_count++] = _strdup(line_buf);
#else
        g_dbg.script_src.lines[g_dbg.script_src.line_count++] = strdup(line_buf);
#endif
    }
    fclose(f);
}

/* ========================================================================= */
/* Breakpoints Handling                                                      */
/* ========================================================================= */

int find_breakpoint(int pc) {
    for (int i = 0; i < g_dbg.bp_count; i++) {
        if (g_dbg.breakpoints[i].pc == pc) return i;
    }
    return -1;
}

void toggle_breakpoint(int pc, int line) {
    int idx = find_breakpoint(pc);
    if (idx >= 0) {
        /* Remove */
        for (int i = idx; i < g_dbg.bp_count - 1; i++) {
            g_dbg.breakpoints[i] = g_dbg.breakpoints[i + 1];
        }
        g_dbg.bp_count--;
        dbg_log_add(2, "[BP] Removed breakpoint at PC %04X (Line %d)", pc, line);
    } else {
        /* Add */
        if (g_dbg.bp_count < MAX_BREAKPOINTS) {
            DebugBreakpoint *bp = &g_dbg.breakpoints[g_dbg.bp_count++];
            bp->pc = pc;
            bp->line = line;
            bp->hit_count = 0;
            bp->enabled = 1;
            dbg_log_add(2, "[BP] Added breakpoint at PC %04X (Line %d)", pc, line);
        }
    }
}

void clear_breakpoints(void) {
    g_dbg.bp_count = 0;
    dbg_log_add(2, "[BP] Cleared all breakpoints");
}

/* ========================================================================= */
/* Call Stack Helper                                                         */
/* ========================================================================= */

int get_call_depth(lua_State *L) {
    int depth = 0;
    for (CallInfo *ci = L->ci; ci != NULL && ci != &L->base_ci; ci = ci->previous) {
        depth++;
    }
    return depth;
}

