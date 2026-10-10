/*
** PyLua Visual Bytecode / CPU Debugger PRO
** Module: Debugger Types & Data Structures
*/

#ifndef DEBUGGER_TYPES_H
#define DEBUGGER_TYPES_H

#include "../../../../include/lprefix.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif
#include <GLFW/glfw3.h>
#include <GL/gl.h>

#include "../../../../include/lua.h"
#include "../../../../include/lauxlib.h"
#include "../../../../include/lualib.h"
#include "../../../../include/lstate.h"
#include "../../../../include/lobject.h"
#include "../../../../include/lopcodes.h"
#include "../../../../include/lopnames.h"
#include "../../../../include/ldebug.h"
#include "../../../../include/lfunc.h"
#include "../../../../include/ltable.h"
#include "../../../../include/lstring.h"
#include "../../../../include/ldo.h"
#include "../../../../include/lvm.h"

#define MAX_BREAKPOINTS 256
#define MAX_LOG_LINES 1024
#define MAX_SCRIPT_LINES 4096

typedef enum {
    DBG_STATUS_PAUSED = 0,
    DBG_STATUS_RUNNING,
    DBG_STATUS_STEP_INTO,
    DBG_STATUS_STEP_OVER,
    DBG_STATUS_TERMINATED
} DbgStatus;

typedef enum {
    TAB_CPU = 0,
    TAB_LOG,
    TAB_BREAKPOINTS,
    TAB_MEMORY,
    TAB_SCRIPT,
    TAB_COUNT
} DbgTab;

typedef struct {
    int pc;
    int line;
    int hit_count;
    int enabled;
} DebugBreakpoint;

typedef struct {
    int pc;
    Instruction raw_inst;
    char hex_bytes[16];
    char opcode_name[24];
    char operands[64];
    char comment[128];
    int line;
    int is_jump;
    int jump_target;
    int is_target;
} DisasmLine;

typedef struct {
    char text[256];
    double timestamp;
    int level; /* 0 = script stdout, 1 = error, 2 = dbg event */
} LogEntry;

typedef struct {
    char *lines[MAX_SCRIPT_LINES];
    int line_count;
    char file_path[260];
} ScriptSourceView;

typedef struct {
    GLFWwindow *window;
    int win_w, win_h;
    DbgStatus status;
    DbgTab active_tab;
    int should_exit;

    /* Target script */
    char script_path[260];
    ScriptSourceView script_src;

    /* Breakpoints */
    DebugBreakpoint breakpoints[MAX_BREAKPOINTS];
    int bp_count;

    /* Logs (Circular Buffer) */
    LogEntry logs[MAX_LOG_LINES];
    int log_start;
    int log_count;
    int log_scroll;

    /* Disassembly & Execution state */
    Proto *current_proto;
    DisasmLine *disasm_lines;
    int disasm_count;
    int current_pc;
    int selected_pc;
    int disasm_scroll_pc;

    /* Step over tracking */
    int step_over_ci_depth;

    /* Registers & stack */
    int reg_scroll;
    int k_scroll;
    int src_scroll;

    /* Mouse & UI interaction */
    double mouse_x, mouse_y;
    int mouse_left_pressed;
    int mouse_left_down;

    /* Scrollbar drag interaction */
    int disasm_sb_dragging;
    float disasm_sb_drag_offset_y;
    int reg_sb_dragging;
    float reg_sb_drag_offset_y;
    int log_sb_dragging;
    float log_sb_drag_offset_y;
    int src_sb_dragging;
    float src_sb_drag_offset_y;

    /* Execution speed & UI refresh */
    double last_ui_update;
    int execution_delay_ms;
    int restart_requested;

    /* Statistics */
    long long instructions_executed;
    double start_time;
    char status_msg[128];
} DebuggerState;

#ifdef _WIN32
typedef struct {
    int f5_down;
    int space_down;
    int pause_down;
    int p_down;
    int f12_down;
    int f7_down;
    int f11_down;
    int f8_down;
    int f10_down;
    int f6_down;
    int f2_down;
    int f9_down;
    int k1_down;
    int k2_down;
    int k3_down;
    int k4_down;
    int k5_down;
    int up_down;
    int down_down;
    int pgup_down;
    int pgdn_down;
} HotkeyState;
#endif

#endif /* DEBUGGER_TYPES_H */

