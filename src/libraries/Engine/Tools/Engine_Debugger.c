/*
** PyLua Engine - Visual CPU / Bytecode Debugger PRO
** Modeled after x64dbg / OllyDbg CPU architecture.
** Provides live bytecode disassembly, jump arrows, execution pointer,
** register inspection, constants pool, breakpoints, source view, and console log.
*/

#define engine_debugger_c
#define LUA_CORE

#include "../../../../include/lprefix.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

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
#include "Engine_Debugger.h"

/* ========================================================================= */
/* 5x7 ASCII Bitmap Font (ASCII 32 ' ' to 126 '~')                           */
/* ========================================================================= */

static const unsigned char dbg_font5x7[95][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, /* 32   */
    {0x00, 0x00, 0x5F, 0x00, 0x00}, /* 33 ! */
    {0x00, 0x07, 0x00, 0x07, 0x00}, /* 34 " */
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, /* 35 # */
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, /* 36 $ */
    {0x23, 0x13, 0x08, 0x64, 0x62}, /* 37 % */
    {0x36, 0x49, 0x55, 0x22, 0x50}, /* 38 & */
    {0x00, 0x05, 0x03, 0x00, 0x00}, /* 39 ' */
    {0x00, 0x1C, 0x22, 0x41, 0x00}, /* 40 ( */
    {0x00, 0x41, 0x22, 0x1C, 0x00}, /* 41 ) */
    {0x14, 0x08, 0x3E, 0x08, 0x14}, /* 42 * */
    {0x08, 0x08, 0x3E, 0x08, 0x08}, /* 43 + */
    {0x00, 0x50, 0x30, 0x00, 0x00}, /* 44 , */
    {0x08, 0x08, 0x08, 0x08, 0x08}, /* 45 - */
    {0x00, 0x60, 0x60, 0x00, 0x00}, /* 46 . */
    {0x20, 0x10, 0x08, 0x04, 0x02}, /* 47 / */
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, /* 48 0 */
    {0x00, 0x42, 0x7F, 0x40, 0x00}, /* 49 1 */
    {0x42, 0x61, 0x51, 0x49, 0x46}, /* 50 2 */
    {0x21, 0x41, 0x45, 0x4B, 0x31}, /* 51 3 */
    {0x18, 0x14, 0x12, 0x7F, 0x10}, /* 52 4 */
    {0x27, 0x45, 0x45, 0x45, 0x39}, /* 53 5 */
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, /* 54 6 */
    {0x01, 0x71, 0x09, 0x05, 0x03}, /* 55 7 */
    {0x36, 0x49, 0x49, 0x49, 0x36}, /* 56 8 */
    {0x06, 0x49, 0x49, 0x29, 0x1E}, /* 57 9 */
    {0x00, 0x36, 0x36, 0x00, 0x00}, /* 58 : */
    {0x00, 0x56, 0x36, 0x00, 0x00}, /* 59 ; */
    {0x08, 0x14, 0x22, 0x41, 0x00}, /* 60 < */
    {0x14, 0x14, 0x14, 0x14, 0x14}, /* 61 = */
    {0x00, 0x41, 0x22, 0x14, 0x08}, /* 62 > */
    {0x02, 0x01, 0x51, 0x09, 0x06}, /* 63 ? */
    {0x32, 0x49, 0x79, 0x41, 0x3E}, /* 64 @ */
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, /* 65 A */
    {0x7F, 0x49, 0x49, 0x49, 0x36}, /* 66 B */
    {0x3E, 0x41, 0x41, 0x41, 0x22}, /* 67 C */
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, /* 68 D */
    {0x7F, 0x49, 0x49, 0x49, 0x41}, /* 69 E */
    {0x7F, 0x09, 0x09, 0x09, 0x01}, /* 70 F */
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, /* 71 G */
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, /* 72 H */
    {0x00, 0x41, 0x7F, 0x41, 0x00}, /* 73 I */
    {0x20, 0x40, 0x41, 0x3F, 0x01}, /* 74 J */
    {0x7F, 0x08, 0x14, 0x22, 0x41}, /* 75 K */
    {0x7F, 0x40, 0x40, 0x40, 0x40}, /* 76 L */
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, /* 77 M */
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, /* 78 N */
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, /* 79 O */
    {0x7F, 0x09, 0x09, 0x09, 0x06}, /* 80 P */
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, /* 81 Q */
    {0x7F, 0x09, 0x19, 0x29, 0x46}, /* 82 R */
    {0x46, 0x49, 0x49, 0x49, 0x31}, /* 83 S */
    {0x01, 0x01, 0x7F, 0x01, 0x01}, /* 84 T */
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, /* 85 U */
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, /* 86 V */
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, /* 87 W */
    {0x63, 0x14, 0x08, 0x14, 0x63}, /* 88 X */
    {0x07, 0x08, 0x70, 0x08, 0x07}, /* 89 Y */
    {0x61, 0x51, 0x49, 0x45, 0x43}, /* 90 Z */
    {0x00, 0x7F, 0x41, 0x41, 0x00}, /* 91 [ */
    {0x02, 0x04, 0x08, 0x10, 0x20}, /* 92 \ */
    {0x00, 0x41, 0x41, 0x7F, 0x00}, /* 93 ] */
    {0x04, 0x02, 0x01, 0x02, 0x04}, /* 94 ^ */
    {0x40, 0x40, 0x40, 0x40, 0x40}, /* 95 _ */
    {0x00, 0x01, 0x02, 0x04, 0x00}, /* 96 ` */
    {0x20, 0x54, 0x54, 0x54, 0x78}, /* 97 a */
    {0x7F, 0x48, 0x44, 0x44, 0x38}, /* 98 b */
    {0x38, 0x44, 0x44, 0x44, 0x20}, /* 99 c */
    {0x38, 0x44, 0x44, 0x48, 0x7F}, /* 100 d */
    {0x38, 0x54, 0x54, 0x54, 0x18}, /* 101 e */
    {0x08, 0x7E, 0x09, 0x01, 0x02}, /* 102 f */
    {0x0C, 0x52, 0x52, 0x52, 0x3E}, /* 103 g */
    {0x7F, 0x08, 0x04, 0x04, 0x78}, /* 104 h */
    {0x00, 0x44, 0x7D, 0x40, 0x00}, /* 105 i */
    {0x20, 0x40, 0x44, 0x3D, 0x00}, /* 106 j */
    {0x7F, 0x10, 0x28, 0x44, 0x00}, /* 107 k */
    {0x00, 0x41, 0x7F, 0x40, 0x00}, /* 108 l */
    {0x7C, 0x04, 0x18, 0x04, 0x78}, /* 109 m */
    {0x7C, 0x08, 0x04, 0x04, 0x78}, /* 110 n */
    {0x38, 0x44, 0x44, 0x44, 0x38}, /* 111 o */
    {0x7C, 0x14, 0x14, 0x14, 0x08}, /* 112 p */
    {0x08, 0x14, 0x14, 0x18, 0x7C}, /* 113 q */
    {0x7C, 0x08, 0x04, 0x04, 0x08}, /* 114 r */
    {0x48, 0x54, 0x54, 0x54, 0x20}, /* 115 s */
    {0x04, 0x3F, 0x44, 0x40, 0x20}, /* 116 t */
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, /* 117 u */
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, /* 118 v */
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, /* 119 w */
    {0x44, 0x28, 0x10, 0x28, 0x44}, /* 120 x */
    {0x0C, 0x50, 0x50, 0x50, 0x3C}, /* 121 y */
    {0x44, 0x64, 0x54, 0x4C, 0x44}, /* 122 z */
    {0x00, 0x08, 0x36, 0x41, 0x00}, /* 123 { */
    {0x00, 0x00, 0x77, 0x00, 0x00}, /* 124 | */
    {0x00, 0x41, 0x36, 0x08, 0x00}, /* 125 } */
    {0x08, 0x04, 0x08, 0x10, 0x08}  /* 126 ~ */
};

/* ========================================================================= */
/* Debugger Data Structures                                                  */
/* ========================================================================= */

#define MAX_BREAKPOINTS 256
#define MAX_LOG_LINES   1024
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

    /* Execution speed & UI refresh */
    double last_ui_update;
    int execution_delay_ms;
    int restart_requested;

    /* Statistics */
    long long instructions_executed;
    double start_time;
    char status_msg[128];
} DebuggerState;

static DebuggerState g_dbg = {
    NULL, 1280, 768, DBG_STATUS_PAUSED, TAB_CPU, 0,
    "", { {NULL}, 0, "" },
    {{0, 0, 0, 0}}, 0,
    {{ "", 0.0, 0 }}, 0, 0, 0,
    NULL, NULL, 0, 0, 0, 0,
    0,
    0, 0, 0,
    0.0, 0.0, 0, 0,
    0.0, 0, 0,
    0, 0.0, "Ready"
};

/* ========================================================================= */
/* Logging Helpers                                                           */
/* ========================================================================= */

static void dbg_log_add(int level, const char *fmt, ...) {
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

/* Custom print hook redirecting script stdout to terminal AND debugger */
static int dbg_custom_print(lua_State *L) {
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

static void load_script_source(const char *path) {
    for (int i = 0; i < g_dbg.script_src.line_count; i++) {
        if (g_dbg.script_src.lines[i]) free(g_dbg.script_src.lines[i]);
        g_dbg.script_src.lines[i] = NULL;
    }
    g_dbg.script_src.line_count = 0;
    strncpy(g_dbg.script_src.file_path, path, sizeof(g_dbg.script_src.file_path) - 1);

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
        g_dbg.script_src.lines[g_dbg.script_src.line_count++] = _strdup(line_buf);
    }
    fclose(f);
}

/* ========================================================================= */
/* TValue Inspection                                                         */
/* ========================================================================= */

static void format_tvalue(char *buf, size_t sz, const TValue *v) {
    if (!v) { snprintf(buf, sz, "nil"); return; }
    switch (ttype(v)) {
        case LUA_TNIL:
            snprintf(buf, sz, "nil"); break;
        case LUA_TBOOLEAN:
            snprintf(buf, sz, "%s", ttistrue(v) ? "true" : "false"); break;
        case LUA_TNUMBER:
            if (ttisinteger(v))
                snprintf(buf, sz, "%lld", (long long)ivalue(v));
            else
                snprintf(buf, sz, "%.6g", (double)fltvalue(v));
            break;
        case LUA_TSTRING: {
            size_t len = tsslen(tsvalue(v));
            const char *str = getstr(tsvalue(v));
            if (len > 24)
                snprintf(buf, sz, "\"%.20s...\"", str);
            else
                snprintf(buf, sz, "\"%s\"", str);
            break;
        }
        case LUA_TTABLE:
            snprintf(buf, sz, "table: 0x%p", (void*)hvalue(v)); break;
        case LUA_TFUNCTION:
            snprintf(buf, sz, "function: 0x%p", (void*)clvalue(v)); break;
        case LUA_TTHREAD:
            snprintf(buf, sz, "thread: 0x%p", (void*)thvalue(v)); break;
        case LUA_TUSERDATA:
        case LUA_TLIGHTUSERDATA:
            snprintf(buf, sz, "userdata: 0x%p", (void*)pvalue(v)); break;
        default:
            snprintf(buf, sz, "<value>"); break;
    }
}

static const char *get_tvalue_typename(const TValue *v) {
    if (!v) return "nil";
    switch (ttype(v)) {
        case LUA_TNIL: return "nil";
        case LUA_TBOOLEAN: return "boolean";
        case LUA_TNUMBER: return ttisinteger(v) ? "integer" : "float";
        case LUA_TSTRING: return "string";
        case LUA_TTABLE: return "table";
        case LUA_TFUNCTION: return "function";
        case LUA_TTHREAD: return "thread";
        case LUA_TUSERDATA:
        case LUA_TLIGHTUSERDATA: return "userdata";
        default: return "unknown";
    }
}

/* ========================================================================= */
/* Disassembly Decoder                                                       */
/* ========================================================================= */

static void decode_proto(Proto *p) {
    if (g_dbg.disasm_lines) {
        free(g_dbg.disasm_lines);
        g_dbg.disasm_lines = NULL;
    }
    g_dbg.current_proto = p;
    g_dbg.disasm_count = p->sizecode;
    if (p->sizecode <= 0) return;

    g_dbg.disasm_lines = (DisasmLine *)calloc((size_t)p->sizecode, sizeof(DisasmLine));
    if (!g_dbg.disasm_lines) return;

    for (int pc = 0; pc < p->sizecode; pc++) {
        DisasmLine *dl = &g_dbg.disasm_lines[pc];
        Instruction i = p->code[pc];
        OpCode op = GET_OPCODE(i);
        dl->pc = pc;
        dl->raw_inst = i;
        dl->line = luaG_getfuncline(p, pc);
        dl->is_jump = 0;
        dl->jump_target = -1;

        /* Raw Hex Bytes */
        snprintf(dl->hex_bytes, sizeof(dl->hex_bytes), "%02X %02X %02X %02X",
                 (unsigned char)(i & 0xFF),
                 (unsigned char)((i >> 8) & 0xFF),
                 (unsigned char)((i >> 16) & 0xFF),
                 (unsigned char)((i >> 24) & 0xFF));

        /* Opcode Name */
        if (op < (int)(sizeof(opnames) / sizeof(opnames[0]))) {
            snprintf(dl->opcode_name, sizeof(dl->opcode_name), "OP_%s", opnames[op]);
        } else {
            snprintf(dl->opcode_name, sizeof(dl->opcode_name), "OP_UNKNOWN_%d", op);
        }

        int a = GETARG_A(i);
        dl->operands[0] = '\0';
        dl->comment[0] = '\0';

        switch (op) {
            case OP_MOVE:
                snprintf(dl->operands, sizeof(dl->operands), "R(%d), R(%d)", a, GETARG_B(i));
                break;
            case OP_LOADI:
                snprintf(dl->operands, sizeof(dl->operands), "R(%d), %d", a, GETARG_sBx(i));
                break;
            case OP_LOADF:
                snprintf(dl->operands, sizeof(dl->operands), "R(%d), %d", a, GETARG_sBx(i));
                break;
            case OP_LOADK: {
                int bx = GETARG_Bx(i);
                snprintf(dl->operands, sizeof(dl->operands), "R(%d), K(%d)", a, bx);
                if (bx >= 0 && bx < p->sizek) {
                    char kstr[64]; format_tvalue(kstr, sizeof(kstr), &p->k[bx]);
                    snprintf(dl->comment, sizeof(dl->comment), "; %s", kstr);
                }
                break;
            }
            case OP_LOADKX:
                snprintf(dl->operands, sizeof(dl->operands), "R(%d)", a);
                break;
            case OP_LOADFALSE:
            case OP_LFALSESKIP:
            case OP_LOADTRUE:
                snprintf(dl->operands, sizeof(dl->operands), "R(%d)", a);
                break;
            case OP_LOADNIL:
                snprintf(dl->operands, sizeof(dl->operands), "R(%d), %d", a, GETARG_B(i));
                break;
            case OP_GETUPVAL:
            case OP_SETUPVAL: {
                int b = GETARG_B(i);
                snprintf(dl->operands, sizeof(dl->operands), "R(%d), Up(%d)", a, b);
                if (b >= 0 && b < p->sizeupvalues && p->upvalues[b].name) {
                    snprintf(dl->comment, sizeof(dl->comment), "; Up[%d]: %s", b, getstr(p->upvalues[b].name));
                }
                break;
            }
            case OP_GETTABUP: {
                int b = GETARG_B(i);
                int c = GETARG_C(i);
                snprintf(dl->operands, sizeof(dl->operands), "R(%d), Up(%d), K(%d)", a, b, c);
                if (c >= 0 && c < p->sizek && ttisstring(&p->k[c])) {
                    snprintf(dl->comment, sizeof(dl->comment), "; Up[%d][\"%s\"]", b, getstr(tsvalue(&p->k[c])));
                }
                break;
            }
            case OP_GETTABLE:
                snprintf(dl->operands, sizeof(dl->operands), "R(%d), R(%d), R(%d)", a, GETARG_B(i), GETARG_C(i));
                break;
            case OP_GETI:
                snprintf(dl->operands, sizeof(dl->operands), "R(%d), R(%d), %d", a, GETARG_B(i), GETARG_C(i));
                break;
            case OP_GETFIELD: {
                int b = GETARG_B(i);
                int c = GETARG_C(i);
                snprintf(dl->operands, sizeof(dl->operands), "R(%d), R(%d), K(%d)", a, b, c);
                if (c >= 0 && c < p->sizek && ttisstring(&p->k[c])) {
                    snprintf(dl->comment, sizeof(dl->comment), "; R(%d).%s", b, getstr(tsvalue(&p->k[c])));
                }
                break;
            }
            case OP_SETTABUP: {
                int b = GETARG_B(i);
                int c = GETARG_C(i);
                int k = GETARG_k(i);
                snprintf(dl->operands, sizeof(dl->operands), "Up(%d), K(%d), %s(%d)", a, b, k ? "K" : "R", c);
                if (b >= 0 && b < p->sizek && ttisstring(&p->k[b])) {
                    snprintf(dl->comment, sizeof(dl->comment), "; Up[%d][\"%s\"] = %s(%d)", a, getstr(tsvalue(&p->k[b])), k ? "K" : "R", c);
                }
                break;
            }
            case OP_SETTABLE:
                snprintf(dl->operands, sizeof(dl->operands), "R(%d), R(%d), %s(%d)", a, GETARG_B(i), GETARG_k(i) ? "K" : "R", GETARG_C(i));
                break;
            case OP_SETI:
                snprintf(dl->operands, sizeof(dl->operands), "R(%d), %d, %s(%d)", a, GETARG_B(i), GETARG_k(i) ? "K" : "R", GETARG_C(i));
                break;
            case OP_SETFIELD: {
                int b = GETARG_B(i);
                int c = GETARG_C(i);
                int k = GETARG_k(i);
                snprintf(dl->operands, sizeof(dl->operands), "R(%d), K(%d), %s(%d)", a, b, k ? "K" : "R", c);
                if (b >= 0 && b < p->sizek && ttisstring(&p->k[b])) {
                    snprintf(dl->comment, sizeof(dl->comment), "; R(%d).%s = %s(%d)", a, getstr(tsvalue(&p->k[b])), k ? "K" : "R", c);
                }
                break;
            }
            case OP_NEWTABLE:
                snprintf(dl->operands, sizeof(dl->operands), "R(%d), %d, %d", a, GETARG_vB(i), GETARG_vC(i));
                break;
            case OP_SELF: {
                int b = GETARG_B(i);
                int c = GETARG_C(i);
                snprintf(dl->operands, sizeof(dl->operands), "R(%d), R(%d), K(%d)", a, b, c);
                if (c >= 0 && c < p->sizek && ttisstring(&p->k[c])) {
                    snprintf(dl->comment, sizeof(dl->comment), "; method '%s'", getstr(tsvalue(&p->k[c])));
                }
                break;
            }
            case OP_ADDI:
                snprintf(dl->operands, sizeof(dl->operands), "R(%d), R(%d), %d", a, GETARG_B(i), GETARG_sC(i));
                break;
            case OP_ADDK: case OP_SUBK: case OP_MULK: case OP_MODK:
            case OP_POWK: case OP_DIVK: case OP_IDIVK: case OP_BANDK:
            case OP_BORK: case OP_BXORK: {
                int b = GETARG_B(i);
                int c = GETARG_C(i);
                snprintf(dl->operands, sizeof(dl->operands), "R(%d), R(%d), K(%d)", a, b, c);
                if (c >= 0 && c < p->sizek) {
                    char kstr[32]; format_tvalue(kstr, sizeof(kstr), &p->k[c]);
                    snprintf(dl->comment, sizeof(dl->comment), "; %s", kstr);
                }
                break;
            }
            case OP_SHLI: case OP_SHRI:
                snprintf(dl->operands, sizeof(dl->operands), "R(%d), R(%d), %d", a, GETARG_B(i), GETARG_sC(i));
                break;
            case OP_ADD: case OP_SUB: case OP_MUL: case OP_MOD:
            case OP_POW: case OP_DIV: case OP_IDIV: case OP_BAND:
            case OP_BOR: case OP_BXOR: case OP_SHL: case OP_SHR:
                snprintf(dl->operands, sizeof(dl->operands), "R(%d), R(%d), R(%d)", a, GETARG_B(i), GETARG_C(i));
                break;
            case OP_UNM: case OP_BNOT: case OP_NOT: case OP_LEN:
                snprintf(dl->operands, sizeof(dl->operands), "R(%d), R(%d)", a, GETARG_B(i));
                break;
            case OP_CONCAT:
                snprintf(dl->operands, sizeof(dl->operands), "R(%d), %d", a, GETARG_B(i));
                break;
            case OP_CLOSE: case OP_TBC:
                snprintf(dl->operands, sizeof(dl->operands), "R(%d)", a);
                break;
            case OP_JMP: {
                int sj = GETARG_sJ(i);
                int target = pc + 1 + sj;
                dl->is_jump = 1;
                dl->jump_target = target;
                snprintf(dl->operands, sizeof(dl->operands), "%+d (to %04X)", sj, target);
                snprintf(dl->comment, sizeof(dl->comment), "; branch to address %04X", target);
                break;
            }
            case OP_EQ: case OP_LT: case OP_LE:
                snprintf(dl->operands, sizeof(dl->operands), "R(%d), R(%d), k=%d", a, GETARG_B(i), GETARG_k(i));
                break;
            case OP_EQK:
                snprintf(dl->operands, sizeof(dl->operands), "R(%d), K(%d), k=%d", a, GETARG_B(i), GETARG_k(i));
                break;
            case OP_EQI: case OP_LTI: case OP_LEI: case OP_GTI: case OP_GEI:
                snprintf(dl->operands, sizeof(dl->operands), "R(%d), %d, k=%d", a, GETARG_sB(i), GETARG_k(i));
                break;
            case OP_TEST:
                snprintf(dl->operands, sizeof(dl->operands), "R(%d), k=%d", a, GETARG_k(i));
                break;
            case OP_TESTSET:
                snprintf(dl->operands, sizeof(dl->operands), "R(%d), R(%d), k=%d", a, GETARG_B(i), GETARG_k(i));
                break;
            case OP_CALL: {
                int b = GETARG_B(i);
                int c = GETARG_C(i);
                snprintf(dl->operands, sizeof(dl->operands), "R(%d), %d, %d", a, b, c);
                snprintf(dl->comment, sizeof(dl->comment), "; %d args, %d returns", b - 1, c - 1);
                break;
            }
            case OP_TAILCALL:
                snprintf(dl->operands, sizeof(dl->operands), "R(%d), %d, %d", a, GETARG_B(i), GETARG_C(i));
                break;
            case OP_RETURN:
                snprintf(dl->operands, sizeof(dl->operands), "R(%d), %d", a, GETARG_B(i));
                break;
            case OP_RETURN0:
                /* No args */
                break;
            case OP_RETURN1:
                snprintf(dl->operands, sizeof(dl->operands), "R(%d)", a);
                break;
            case OP_FORLOOP: {
                int bx = GETARG_Bx(i);
                int target = pc + 1 - bx;
                dl->is_jump = 1;
                dl->jump_target = target;
                snprintf(dl->operands, sizeof(dl->operands), "R(%d), -%d (to %04X)", a, bx, target);
                snprintf(dl->comment, sizeof(dl->comment), "; loop back to %04X", target);
                break;
            }
            case OP_FORPREP: {
                int bx = GETARG_Bx(i);
                int target = pc + 1 + bx + 1;
                dl->is_jump = 1;
                dl->jump_target = target;
                snprintf(dl->operands, sizeof(dl->operands), "R(%d), +%d (to %04X)", a, bx + 1, target);
                break;
            }
            case OP_TFORPREP: {
                int bx = GETARG_Bx(i);
                int target = pc + 1 + bx;
                dl->is_jump = 1;
                dl->jump_target = target;
                snprintf(dl->operands, sizeof(dl->operands), "R(%d), +%d (to %04X)", a, bx, target);
                break;
            }
            case OP_TFORCALL:
                snprintf(dl->operands, sizeof(dl->operands), "R(%d), %d", a, GETARG_C(i));
                break;
            case OP_TFORLOOP: {
                int bx = GETARG_Bx(i);
                int target = pc + 1 - bx;
                dl->is_jump = 1;
                dl->jump_target = target;
                snprintf(dl->operands, sizeof(dl->operands), "R(%d), -%d (to %04X)", a, bx, target);
                break;
            }
            case OP_SETLIST:
                snprintf(dl->operands, sizeof(dl->operands), "R(%d), %d, %d", a, GETARG_vB(i), GETARG_vC(i));
                break;
            case OP_CLOSURE:
                snprintf(dl->operands, sizeof(dl->operands), "R(%d), Proto(%d)", a, GETARG_Bx(i));
                break;
            case OP_VARARG:
                snprintf(dl->operands, sizeof(dl->operands), "R(%d), %d", a, GETARG_C(i));
                break;
            case OP_VARARGPREP:
                snprintf(dl->operands, sizeof(dl->operands), "%d", a);
                break;
            case OP_EXTRAARG:
                snprintf(dl->operands, sizeof(dl->operands), "%d", GETARG_Ax(i));
                break;
            default:
                snprintf(dl->operands, sizeof(dl->operands), "A:%d", a);
                break;
        }
    }

    /* Mark jump targets */
    for (int pc = 0; pc < p->sizecode; pc++) {
        if (g_dbg.disasm_lines[pc].is_jump) {
            int t = g_dbg.disasm_lines[pc].jump_target;
            if (t >= 0 && t < p->sizecode) {
                g_dbg.disasm_lines[t].is_target = 1;
            }
        }
    }
}

/* ========================================================================= */
/* Breakpoints Handling                                                      */
/* ========================================================================= */

static int find_breakpoint(int pc) {
    for (int i = 0; i < g_dbg.bp_count; i++) {
        if (g_dbg.breakpoints[i].pc == pc) return i;
    }
    return -1;
}

static void toggle_breakpoint(int pc, int line) {
    int idx = find_breakpoint(pc);
    if (idx >= 0) {
        for (int i = idx; i < g_dbg.bp_count - 1; i++) {
            g_dbg.breakpoints[i] = g_dbg.breakpoints[i + 1];
        }
        g_dbg.bp_count--;
        dbg_log_add(2, "[BP] Removed breakpoint at PC %04X", pc);
    } else if (g_dbg.bp_count < MAX_BREAKPOINTS) {
        g_dbg.breakpoints[g_dbg.bp_count].pc = pc;
        g_dbg.breakpoints[g_dbg.bp_count].line = line;
        g_dbg.breakpoints[g_dbg.bp_count].hit_count = 0;
        g_dbg.breakpoints[g_dbg.bp_count].enabled = 1;
        g_dbg.bp_count++;
        dbg_log_add(2, "[BP] Set breakpoint at PC %04X (Line %d)", pc, line);
    }
}

/* ========================================================================= */
/* 2D OpenGL GUI Rendering Primitives                                        */
/* ========================================================================= */

static void dbg_draw_rect(float x, float y, float w, float h, float r, float g, float b, float a) {
    glColor4f(r, g, b, a);
    glBegin(GL_QUADS);
        glVertex2f(x, y);
        glVertex2f(x + w, y);
        glVertex2f(x + w, y + h);
        glVertex2f(x, y + h);
    glEnd();
}

static void dbg_draw_rect_outline(float x, float y, float w, float h, float lw, float r, float g, float b, float a) {
    glColor4f(r, g, b, a);
    glLineWidth(lw);
    glBegin(GL_LINE_LOOP);
        glVertex2f(x, y);
        glVertex2f(x + w, y);
        glVertex2f(x + w, y + h);
        glVertex2f(x, y + h);
    glEnd();
}

static void dbg_draw_line(float x1, float y1, float x2, float y2, float lw, float r, float g, float b, float a) {
    glColor4f(r, g, b, a);
    glLineWidth(lw);
    glBegin(GL_LINES);
        glVertex2f(x1, y1);
        glVertex2f(x2, y2);
    glEnd();
}

static void dbg_draw_triangle(float x1, float y1, float x2, float y2, float x3, float y3, float r, float g, float b, float a) {
    glColor4f(r, g, b, a);
    glBegin(GL_TRIANGLES);
        glVertex2f(x1, y1);
        glVertex2f(x2, y2);
        glVertex2f(x3, y3);
    glEnd();
}

static void dbg_draw_circle(float cx, float cy, float radius, float r, float g, float b, float a) {
    glColor4f(r, g, b, a);
    glBegin(GL_TRIANGLE_FAN);
        glVertex2f(cx, cy);
        for (int i = 0; i <= 20; i++) {
            float theta = (float)i * 2.0f * 3.14159265f / 20.0f;
            glVertex2f(cx + cosf(theta) * radius, cy + sinf(theta) * radius);
        }
    glEnd();
}

static void dbg_draw_text(float x, float y, const char *str, float r, float g, float b, float scale) {
    if (!str) return;
    glColor4f(r, g, b, 1.0f);
    float cur_x = x;

    for (int i = 0; str[i] != '\0'; i++) {
        unsigned char c = (unsigned char)str[i];
        if (c < 32 || c > 126) c = '?';
        const unsigned char *glyph = dbg_font5x7[c - 32];

        glBegin(GL_QUADS);
        for (int col = 0; col < 5; col++) {
            unsigned char col_bits = glyph[col];
            for (int row = 0; row < 7; row++) {
                if (col_bits & (1 << row)) {
                    float px = cur_x + (float)col * scale;
                    float py = y + (float)row * scale;
                    glVertex2f(px, py);
                    glVertex2f(px + scale, py);
                    glVertex2f(px + scale, py + scale);
                    glVertex2f(px, py + scale);
                }
            }
        }
        glEnd();
        cur_x += 6.0f * scale;
    }
}

static int dbg_button(float x, float y, float w, float h, const char *text, float accent_r, float accent_g, float accent_b) {
    int hover = (g_dbg.mouse_x >= x && g_dbg.mouse_x <= x + w && g_dbg.mouse_y >= y && g_dbg.mouse_y <= y + h);
    float bg_r = hover ? 0.22f : 0.16f;
    float bg_g = hover ? 0.22f : 0.16f;
    float bg_b = hover ? 0.25f : 0.18f;

    dbg_draw_rect(x, y, w, h, bg_r, bg_g, bg_b, 1.0f);
    dbg_draw_rect(x, y + h - 2.0f, w, 2.0f, accent_r, accent_g, accent_b, 1.0f);
    dbg_draw_rect_outline(x, y, w, h, 1.0f, 0.3f, 0.3f, 0.35f, 1.0f);

    float text_x = x + (w - (float)strlen(text) * 7.5f) * 0.5f;
    float text_y = y + (h - 10.0f) * 0.5f;
    dbg_draw_text(text_x, text_y, text, hover ? 1.0f : 0.9f, hover ? 1.0f : 0.9f, hover ? 1.0f : 0.9f, 1.25f);

    return hover && g_dbg.mouse_left_pressed;
}

/* ========================================================================= */
/* Call Stack Helper                                                         */
/* ========================================================================= */

static int get_call_depth(lua_State *L) {
    int depth = 0;
    for (CallInfo *ci = L->ci; ci != NULL && ci != &L->base_ci; ci = ci->previous) {
        depth++;
    }
    return depth;
}

/* ========================================================================= */
/* Main Debugger GUI Drawing                                                 */
/* ========================================================================= */

static void render_debugger_gui(lua_State *L) {
    glClearColor(0.11f, 0.11f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glViewport(0, 0, g_dbg.win_w, g_dbg.win_h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, g_dbg.win_w, g_dbg.win_h, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /* --------------------------------------------------------------------- */
    /* 1. TOP TITLE & TAB BAR (Y: 0 to 28)                                   */
    /* --------------------------------------------------------------------- */
    dbg_draw_rect(0, 0, (float)g_dbg.win_w, 28, 0.13f, 0.13f, 0.15f, 1.0f);
    dbg_draw_line(0, 28, (float)g_dbg.win_w, 28, 1.0f, 0.22f, 0.22f, 0.25f, 1.0f);

    const char *tab_names[] = { "CPU / DISASM", "LOG / CONSOLE", "BREAKPOINTS", "MEMORY / GLOBALS", "SOURCE SCRIPT" };
    float tab_x = 10.0f;
    for (int t = 0; t < TAB_COUNT; t++) {
        float tab_w = 125.0f;
        int active = (g_dbg.active_tab == t);
        int hover = (g_dbg.mouse_x >= tab_x && g_dbg.mouse_x <= tab_x + tab_w && g_dbg.mouse_y >= 0 && g_dbg.mouse_y <= 28);
        if (active) {
            dbg_draw_rect(tab_x, 2, tab_w, 26, 0.18f, 0.18f, 0.22f, 1.0f);
            dbg_draw_rect(tab_x, 0, tab_w, 3, 0.0f, 0.6f, 1.0f, 1.0f);
        } else if (hover) {
            dbg_draw_rect(tab_x, 2, tab_w, 26, 0.15f, 0.15f, 0.18f, 1.0f);
        }
        if (hover && g_dbg.mouse_left_pressed) {
            g_dbg.active_tab = (DbgTab)t;
        }
        dbg_draw_text(tab_x + 10, 8, tab_names[t], active ? 1.0f : 0.7f, active ? 1.0f : 0.7f, active ? 1.0f : 0.7f, 1.15f);
        tab_x += tab_w + 5.0f;
    }

    /* Right title banner */
    dbg_draw_text((float)g_dbg.win_w - 220, 8, "PyLua CPU Debugger PRO", 0.0f, 0.75f, 1.0f, 1.2f);

    /* --------------------------------------------------------------------- */
    /* 2. TOOLBAR & CONTROL BUTTONS (Y: 28 to 62)                            */
    /* --------------------------------------------------------------------- */
    dbg_draw_rect(0, 28, (float)g_dbg.win_w, 34, 0.15f, 0.15f, 0.17f, 1.0f);
    dbg_draw_line(0, 62, (float)g_dbg.win_w, 62, 1.0f, 0.25f, 0.25f, 0.28f, 1.0f);

    float btn_x = 10.0f;
    if (g_dbg.status == DBG_STATUS_RUNNING) {
        if (dbg_button(btn_x, 32, 95, 24, "PAUSE (F5/P)", 0.95f, 0.61f, 0.07f)) {
            g_dbg.status = DBG_STATUS_PAUSED;
            dbg_log_add(2, "[EXEC] Paused by user");
        }
    } else {
        if (dbg_button(btn_x, 32, 95, 24, "RUN (F5)", 0.18f, 0.8f, 0.44f)) {
            g_dbg.status = DBG_STATUS_RUNNING;
            dbg_log_add(2, "[EXEC] Continued execution (F5)");
        }
    }
    btn_x += 102.0f;

    if (dbg_button(btn_x, 32, 95, 24, "STEP IN (F7)", 0.2f, 0.6f, 1.0f)) {
        g_dbg.status = DBG_STATUS_STEP_INTO;
    }
    btn_x += 102.0f;

    if (dbg_button(btn_x, 32, 105, 24, "STEP OVER (F8)", 0.0f, 0.75f, 0.8f)) {
        g_dbg.step_over_ci_depth = get_call_depth(L);
        g_dbg.status = DBG_STATUS_STEP_OVER;
    }
    btn_x += 112.0f;

    if (dbg_button(btn_x, 32, 110, 24, "TOGGLE BP (F2)", 0.9f, 0.25f, 0.2f)) {
        if (g_dbg.selected_pc >= 0 && g_dbg.current_proto) {
            int line = luaG_getfuncline(g_dbg.current_proto, g_dbg.selected_pc);
            toggle_breakpoint(g_dbg.selected_pc, line);
        }
    }
    btn_x += 118.0f;

    /* Speed / Slow-Motion button */
    char speed_lbl[32];
    if (g_dbg.execution_delay_ms == 0) strcpy(speed_lbl, "SPEED: MAX");
    else snprintf(speed_lbl, sizeof(speed_lbl), "SPEED: %dms", g_dbg.execution_delay_ms);

    if (dbg_button(btn_x, 32, 105, 24, speed_lbl, 0.6f, 0.35f, 0.85f)) {
        if (g_dbg.execution_delay_ms == 0) g_dbg.execution_delay_ms = 10;
        else if (g_dbg.execution_delay_ms == 10) g_dbg.execution_delay_ms = 50;
        else if (g_dbg.execution_delay_ms == 50) g_dbg.execution_delay_ms = 100;
        else g_dbg.execution_delay_ms = 0;
        dbg_log_add(2, "[SPEED] Execution speed set to %d ms/inst (F6)", g_dbg.execution_delay_ms);
    }
    btn_x += 112.0f;

    /* Execution State Badge */
    float badge_x = (float)g_dbg.win_w - 230;
    if (g_dbg.status == DBG_STATUS_PAUSED) {
        dbg_draw_rect(badge_x, 32, 220, 24, 0.4f, 0.3f, 0.05f, 1.0f);
        dbg_draw_rect_outline(badge_x, 32, 220, 24, 1.0f, 0.9f, 0.7f, 0.1f, 1.0f);
        char pctxt[64];
        snprintf(pctxt, sizeof(pctxt), "PAUSED @ PC %04X", g_dbg.current_pc);
        dbg_draw_text(badge_x + 15, 38, pctxt, 1.0f, 0.85f, 0.2f, 1.25f);
    } else if (g_dbg.status == DBG_STATUS_RUNNING) {
        dbg_draw_rect(badge_x, 32, 220, 24, 0.08f, 0.35f, 0.15f, 1.0f);
        dbg_draw_rect_outline(badge_x, 32, 220, 24, 1.0f, 0.2f, 0.85f, 0.4f, 1.0f);
        dbg_draw_text(badge_x + 35, 38, "RUNNING...", 0.3f, 1.0f, 0.5f, 1.25f);
    } else if (g_dbg.status == DBG_STATUS_TERMINATED) {
        dbg_draw_rect(badge_x, 32, 220, 24, 0.35f, 0.1f, 0.1f, 1.0f);
        dbg_draw_rect_outline(badge_x, 32, 220, 24, 1.0f, 0.9f, 0.2f, 0.2f, 1.0f);
        dbg_draw_text(badge_x + 30, 38, "TERMINATED", 1.0f, 0.4f, 0.4f, 1.25f);
    }

    /* --------------------------------------------------------------------- */
    /* 3. SUB-HEADER / FUNCTION INFO (Y: 62 to 84)                           */
    /* --------------------------------------------------------------------- */
    dbg_draw_rect(0, 62, (float)g_dbg.win_w, 22, 0.12f, 0.12f, 0.14f, 1.0f);
    dbg_draw_line(0, 84, (float)g_dbg.win_w, 84, 1.0f, 0.22f, 0.22f, 0.25f, 1.0f);

    char info_buf[256];
    const char *src_name = (g_dbg.current_proto && g_dbg.current_proto->source) ? getstr(g_dbg.current_proto->source) : g_dbg.script_path;
    int cur_line = (g_dbg.current_proto) ? luaG_getfuncline(g_dbg.current_proto, g_dbg.current_pc) : 0;
    snprintf(info_buf, sizeof(info_buf), "Source: %s | Line: %d | PC: %04X / %04X | Opcode: %s | Instructions: %lld",
             src_name, cur_line, g_dbg.current_pc, g_dbg.disasm_count,
             (g_dbg.disasm_lines && g_dbg.current_pc >= 0 && g_dbg.current_pc < g_dbg.disasm_count) ? g_dbg.disasm_lines[g_dbg.current_pc].opcode_name : "NONE",
             g_dbg.instructions_executed);
    dbg_draw_text(12, 68, info_buf, 0.75f, 0.8f, 0.85f, 1.15f);

    /* --------------------------------------------------------------------- */
    /* 4. WORKSPACE CONTENT                                                  */
    /* --------------------------------------------------------------------- */
    float main_y = 84.0f;
    float bottom_bar_h = 24.0f;
    float main_h = (float)g_dbg.win_h - main_y - bottom_bar_h;

    if (g_dbg.active_tab == TAB_CPU) {
        /* SPLIT VIEW: Disassembly (Left) + Registers/State (Right) */
        float split_x = (float)g_dbg.win_w - 440.0f;

        /* --- LEFT: DISASSEMBLY TABLE --- */
        /* Table Column Headers (Y: 84 to 106) */
        dbg_draw_rect(0, main_y, split_x, 22, 0.16f, 0.16f, 0.19f, 1.0f);
        dbg_draw_line(0, main_y + 22, split_x, main_y + 22, 1.0f, 0.25f, 0.25f, 0.28f, 1.0f);

        dbg_draw_text(8, main_y + 6, "BP", 0.9f, 0.9f, 0.9f, 1.15f);
        dbg_draw_text(30, main_y + 6, "PTR", 0.9f, 0.9f, 0.9f, 1.15f);
        dbg_draw_text(65, main_y + 6, "ADDRESS", 0.9f, 0.9f, 0.9f, 1.15f);
        dbg_draw_text(130, main_y + 6, "BYTES", 0.9f, 0.9f, 0.9f, 1.15f);
        dbg_draw_text(230, main_y + 6, "FLOW", 0.9f, 0.9f, 0.9f, 1.15f);
        dbg_draw_text(275, main_y + 6, "DISASSEMBLY", 0.9f, 0.9f, 0.9f, 1.15f);
        dbg_draw_text(540, main_y + 6, "COMMENTS / CONSTANTS", 0.9f, 0.9f, 0.9f, 1.15f);

        /* Table Rows */
        float row_y_start = main_y + 22;
        float row_h = 22.0f;
        int max_visible_rows = (int)((main_h - 22) / row_h);

        /* Keep current_pc visible */
        if (g_dbg.current_pc < g_dbg.disasm_scroll_pc) {
            g_dbg.disasm_scroll_pc = g_dbg.current_pc;
        } else if (g_dbg.current_pc >= g_dbg.disasm_scroll_pc + max_visible_rows) {
            g_dbg.disasm_scroll_pc = g_dbg.current_pc - max_visible_rows + 4;
            if (g_dbg.disasm_scroll_pc < 0) g_dbg.disasm_scroll_pc = 0;
        }

        for (int r = 0; r < max_visible_rows; r++) {
            int pc_idx = g_dbg.disasm_scroll_pc + r;
            if (pc_idx >= g_dbg.disasm_count) break;

            float cur_row_y = row_y_start + (float)r * row_h;
            DisasmLine *line = &g_dbg.disasm_lines[pc_idx];
            int is_cur = (pc_idx == g_dbg.current_pc);
            int is_sel = (pc_idx == g_dbg.selected_pc);
            int bp_idx = find_breakpoint(pc_idx);

            /* Zebra striping */
            if (is_cur) {
                dbg_draw_rect(0, cur_row_y, split_x, row_h, 0.12f, 0.28f, 0.45f, 1.0f);
                dbg_draw_rect_outline(0, cur_row_y, split_x, row_h, 1.0f, 0.0f, 0.65f, 1.0f, 1.0f);
            } else if (is_sel) {
                dbg_draw_rect(0, cur_row_y, split_x, row_h, 0.22f, 0.24f, 0.28f, 1.0f);
                dbg_draw_rect_outline(0, cur_row_y, split_x, row_h, 1.0f, 0.4f, 0.5f, 0.6f, 1.0f);
            } else if (r % 2 == 1) {
                dbg_draw_rect(0, cur_row_y, split_x, row_h, 0.13f, 0.13f, 0.15f, 1.0f);
            }

            /* Click handling for row selection and breakpoint toggle */
            if (g_dbg.mouse_left_pressed && g_dbg.mouse_y >= cur_row_y && g_dbg.mouse_y < cur_row_y + row_h && g_dbg.mouse_x < split_x) {
                g_dbg.selected_pc = pc_idx;
                if (g_dbg.mouse_x < 30.0f) {
                    toggle_breakpoint(pc_idx, line->line);
                }
            }

            /* Column 1: Breakpoint Indicator (Red Dot) */
            if (bp_idx >= 0) {
                dbg_draw_circle(14.0f, cur_row_y + row_h * 0.5f, 5.0f, 0.95f, 0.2f, 0.2f, 1.0f);
                dbg_draw_circle(14.0f, cur_row_y + row_h * 0.5f, 2.0f, 1.0f, 0.8f, 0.8f, 1.0f);
            }

            /* Column 2: Execution Pointer Arrow (▶ PC) */
            if (is_cur) {
                dbg_draw_triangle(32.0f, cur_row_y + 4.0f,
                                  32.0f, cur_row_y + row_h - 4.0f,
                                  44.0f, cur_row_y + row_h * 0.5f,
                                  1.0f, 0.9f, 0.1f, 1.0f);
            }

            /* Column 3: Address */
            char addr_str[16];
            snprintf(addr_str, sizeof(addr_str), "%04X", pc_idx);
            dbg_draw_text(65, cur_row_y + 5, addr_str, 0.45f, 0.85f, 0.55f, 1.25f);

            /* Column 4: Raw Hex Bytes */
            dbg_draw_text(130, cur_row_y + 5, line->hex_bytes, 0.55f, 0.75f, 0.95f, 1.2f);

            /* Column 5 & 6: Instruction Opcode + Operands */
            dbg_draw_text(275, cur_row_y + 5, line->opcode_name, 1.0f, 0.85f, 0.4f, 1.25f);
            dbg_draw_text(375, cur_row_y + 5, line->operands, 0.85f, 0.85f, 0.9f, 1.2f);

            /* Column 7: Comments / Evaluated Constants */
            if (line->comment[0] != '\0') {
                dbg_draw_text(540, cur_row_y + 5, line->comment, 0.4f, 0.75f, 0.45f, 1.2f);
            }
        }

        /* Draw Jump / Branch Arrows connecting jumps to destinations (matching user's x64dbg screenshot!) */
        for (int r = 0; r < max_visible_rows; r++) {
            int pc_idx = g_dbg.disasm_scroll_pc + r;
            if (pc_idx >= g_dbg.disasm_count) break;
            DisasmLine *line = &g_dbg.disasm_lines[pc_idx];
            if (!line->is_jump || line->jump_target < 0) continue;

            int target_idx = line->jump_target;
            float src_y = row_y_start + (float)r * row_h + row_h * 0.5f;
            float branch_x = 245.0f;
            int is_active_branch = (pc_idx == g_dbg.current_pc);
            float arr_r = is_active_branch ? 1.0f : 0.85f;
            float arr_g = is_active_branch ? 0.3f : 0.2f;
            float arr_b = is_active_branch ? 0.2f : 0.2f;
            float line_w = is_active_branch ? 2.5f : 1.5f;

            /* Check if target is visible */
            int target_r = target_idx - g_dbg.disasm_scroll_pc;
            if (target_r >= 0 && target_r < max_visible_rows) {
                float dst_y = row_y_start + (float)target_r * row_h + row_h * 0.5f;
                /* Draw curved / bracket line from src to dst */
                dbg_draw_line(260.0f, src_y, branch_x, src_y, line_w, arr_r, arr_g, arr_b, 1.0f);
                dbg_draw_line(branch_x, src_y, branch_x, dst_y, line_w, arr_r, arr_g, arr_b, 1.0f);
                dbg_draw_line(branch_x, dst_y, 258.0f, dst_y, line_w, arr_r, arr_g, arr_b, 1.0f);
                /* Arrow tip pointing to destination */
                dbg_draw_triangle(258.0f, dst_y - 4.0f, 258.0f, dst_y + 4.0f, 266.0f, dst_y, arr_r, arr_g, arr_b, 1.0f);
            } else {
                /* Target is off-screen: draw arrow pointing up or down */
                dbg_draw_line(260.0f, src_y, branch_x, src_y, line_w, arr_r, arr_g, arr_b, 1.0f);
                if (target_idx < pc_idx) {
                    /* Jump backward (up) */
                    dbg_draw_line(branch_x, src_y, branch_x, row_y_start, line_w, arr_r, arr_g, arr_b, 1.0f);
                    dbg_draw_triangle(branch_x - 4.0f, row_y_start + 8.0f, branch_x + 4.0f, row_y_start + 8.0f, branch_x, row_y_start, arr_r, arr_g, arr_b, 1.0f);
                } else {
                    /* Jump forward (down) */
                    float max_y = row_y_start + (float)max_visible_rows * row_h;
                    dbg_draw_line(branch_x, src_y, branch_x, max_y, line_w, arr_r, arr_g, arr_b, 1.0f);
                    dbg_draw_triangle(branch_x - 4.0f, max_y - 8.0f, branch_x + 4.0f, max_y - 8.0f, branch_x, max_y, arr_r, arr_g, arr_b, 1.0f);
                }
            }
        }

        /* --- RIGHT PANEL: REGISTERS & CONSTANTS --- */
        dbg_draw_line(split_x, main_y, split_x, (float)g_dbg.win_h - bottom_bar_h, 1.0f, 0.28f, 0.28f, 0.32f, 1.0f);
        dbg_draw_rect(split_x, main_y, 440, main_h, 0.12f, 0.12f, 0.14f, 1.0f);

        /* Section 1: Virtual Registers (R0..RN) */
        float right_y = main_y;
        dbg_draw_rect(split_x, right_y, 440, 22, 0.17f, 0.17f, 0.21f, 1.0f);
        dbg_draw_line(split_x, right_y + 22, (float)g_dbg.win_w, right_y + 22, 1.0f, 0.25f, 0.25f, 0.28f, 1.0f);
        dbg_draw_text(split_x + 10, right_y + 6, "REGISTERS (R0 .. RN)", 0.3f, 0.85f, 1.0f, 1.15f);

        right_y += 24.0f;
        int max_regs = (g_dbg.current_proto) ? g_dbg.current_proto->maxstacksize : 16;
        if (max_regs > 32) max_regs = 32;

        CallInfo *ci = L->ci;
        StkId base = (ci && isLua(ci)) ? (ci->func.p + 1) : NULL;

        float reg_box_h = 240.0f;
        int visible_regs = (int)(reg_box_h / 20.0f);
        for (int r = 0; r < visible_regs; r++) {
            int reg_idx = g_dbg.reg_scroll + r;
            if (reg_idx >= max_regs) break;
            float r_row_y = right_y + (float)r * 20.0f;

            if (r % 2 == 1) {
                dbg_draw_rect(split_x, r_row_y, 440, 20, 0.14f, 0.14f, 0.16f, 1.0f);
            }

            char r_lbl[32];
            snprintf(r_lbl, sizeof(r_lbl), "R[%d]:", reg_idx);
            dbg_draw_text(split_x + 12, r_row_y + 4, r_lbl, 0.95f, 0.75f, 0.3f, 1.15f);

            if (base && (base + reg_idx < L->top.p)) {
                TValue *val = s2v(base + reg_idx);
                char val_str[128];
                format_tvalue(val_str, sizeof(val_str), val);
                const char *type_name = get_tvalue_typename(val);
                dbg_draw_text(split_x + 80, r_row_y + 4, val_str, 0.9f, 0.9f, 0.95f, 1.15f);

                char t_str[32];
                snprintf(t_str, sizeof(t_str), "(%s)", type_name);
                dbg_draw_text(split_x + 320, r_row_y + 4, t_str, 0.5f, 0.75f, 0.6f, 1.05f);
            } else {
                dbg_draw_text(split_x + 80, r_row_y + 4, "nil / empty", 0.45f, 0.45f, 0.5f, 1.15f);
            }
        }

        /* Section 2: Constants Pool (K0..KM) */
        right_y += reg_box_h + 5.0f;
        dbg_draw_rect(split_x, right_y, 440, 22, 0.17f, 0.17f, 0.21f, 1.0f);
        dbg_draw_line(split_x, right_y + 22, (float)g_dbg.win_w, right_y + 22, 1.0f, 0.25f, 0.25f, 0.28f, 1.0f);
        dbg_draw_text(split_x + 10, right_y + 6, "CONSTANTS POOL (K0 .. KM)", 0.4f, 0.9f, 0.5f, 1.15f);

        right_y += 24.0f;
        int num_k = (g_dbg.current_proto) ? g_dbg.current_proto->sizek : 0;
        float k_box_h = 160.0f;
        int visible_k = (int)(k_box_h / 20.0f);
        for (int k = 0; k < visible_k; k++) {
            int k_idx = g_dbg.k_scroll + k;
            if (k_idx >= num_k) break;
            float k_row_y = right_y + (float)k * 20.0f;

            if (k % 2 == 1) {
                dbg_draw_rect(split_x, k_row_y, 440, 20, 0.14f, 0.14f, 0.16f, 1.0f);
            }

            char k_lbl[32];
            snprintf(k_lbl, sizeof(k_lbl), "K[%d]:", k_idx);
            dbg_draw_text(split_x + 12, k_row_y + 4, k_lbl, 0.4f, 0.85f, 0.55f, 1.15f);

            if (g_dbg.current_proto) {
                TValue *kv = &g_dbg.current_proto->k[k_idx];
                char val_str[128];
                format_tvalue(val_str, sizeof(val_str), kv);
                dbg_draw_text(split_x + 80, k_row_y + 4, val_str, 0.85f, 0.85f, 0.9f, 1.15f);
            }
        }

        /* Section 3: Upvalues & Call Stack Summary */
        right_y += k_box_h + 5.0f;
        dbg_draw_rect(split_x, right_y, 440, 22, 0.17f, 0.17f, 0.21f, 1.0f);
        dbg_draw_line(split_x, right_y + 22, (float)g_dbg.win_w, right_y + 22, 1.0f, 0.25f, 0.25f, 0.28f, 1.0f);
        dbg_draw_text(split_x + 10, right_y + 6, "UPVALUES & CALL STACK", 0.95f, 0.5f, 0.85f, 1.15f);

        right_y += 24.0f;
        int num_ups = (g_dbg.current_proto) ? g_dbg.current_proto->sizeupvalues : 0;
        for (int u = 0; u < num_ups && u < 6; u++) {
            float u_row_y = right_y + (float)u * 20.0f;
            char u_lbl[64];
            const char *uname = (g_dbg.current_proto->upvalues[u].name) ? getstr(g_dbg.current_proto->upvalues[u].name) : "(unnamed)";
            snprintf(u_lbl, sizeof(u_lbl), "Up[%d] %s (instack=%d, idx=%d)", u, uname,
                     g_dbg.current_proto->upvalues[u].instack, g_dbg.current_proto->upvalues[u].idx);
            dbg_draw_text(split_x + 12, u_row_y + 4, u_lbl, 0.9f, 0.7f, 0.9f, 1.1f);
        }
    } else if (g_dbg.active_tab == TAB_LOG) {
        /* FULL LOG & CONSOLE VIEW */
        dbg_draw_rect(0, main_y, (float)g_dbg.win_w, 22, 0.16f, 0.16f, 0.19f, 1.0f);
        dbg_draw_text(15, main_y + 6, "TIMESTAMP", 0.9f, 0.9f, 0.9f, 1.15f);
        dbg_draw_text(120, main_y + 6, "LEVEL", 0.9f, 0.9f, 0.9f, 1.15f);
        dbg_draw_text(220, main_y + 6, "CONSOLE OUTPUT / EVENT MESSAGE", 0.9f, 0.9f, 0.9f, 1.15f);

        float log_y = main_y + 24.0f;
        int visible_logs = (int)((main_h - 26) / 20.0f);
        for (int i = 0; i < visible_logs; i++) {
            int log_idx = g_dbg.log_scroll + i;
            if (log_idx >= g_dbg.log_count) break;
            LogEntry *e = &g_dbg.logs[(g_dbg.log_start + log_idx) % MAX_LOG_LINES];
            float cur_ly = log_y + (float)i * 20.0f;

            if (i % 2 == 1) dbg_draw_rect(0, cur_ly, (float)g_dbg.win_w, 20, 0.13f, 0.13f, 0.15f, 1.0f);

            char time_str[32];
            snprintf(time_str, sizeof(time_str), "[%06.2fs]", e->timestamp);
            dbg_draw_text(15, cur_ly + 4, time_str, 0.6f, 0.65f, 0.7f, 1.15f);

            if (e->level == 0) {
                dbg_draw_text(120, cur_ly + 4, "STDOUT", 0.2f, 0.8f, 0.4f, 1.15f);
                dbg_draw_text(220, cur_ly + 4, e->text, 0.9f, 0.95f, 1.0f, 1.2f);
            } else if (e->level == 1) {
                dbg_draw_text(120, cur_ly + 4, "ERROR", 1.0f, 0.3f, 0.3f, 1.15f);
                dbg_draw_text(220, cur_ly + 4, e->text, 1.0f, 0.5f, 0.5f, 1.2f);
            } else {
                dbg_draw_text(120, cur_ly + 4, "DEBUG", 0.0f, 0.75f, 1.0f, 1.15f);
                dbg_draw_text(220, cur_ly + 4, e->text, 0.6f, 0.85f, 1.0f, 1.2f);
            }
        }
    } else if (g_dbg.active_tab == TAB_BREAKPOINTS) {
        /* BREAKPOINTS LIST VIEW */
        dbg_draw_rect(0, main_y, (float)g_dbg.win_w, 22, 0.16f, 0.16f, 0.19f, 1.0f);
        dbg_draw_text(15, main_y + 6, "#", 0.9f, 0.9f, 0.9f, 1.15f);
        dbg_draw_text(50, main_y + 6, "STATUS", 0.9f, 0.9f, 0.9f, 1.15f);
        dbg_draw_text(140, main_y + 6, "PC OFFSET", 0.9f, 0.9f, 0.9f, 1.15f);
        dbg_draw_text(260, main_y + 6, "SOURCE LINE", 0.9f, 0.9f, 0.9f, 1.15f);
        dbg_draw_text(380, main_y + 6, "HIT COUNT", 0.9f, 0.9f, 0.9f, 1.15f);

        float bp_y = main_y + 24.0f;
        if (g_dbg.bp_count == 0) {
            dbg_draw_text(30, bp_y + 20, "No breakpoints set. Press F2 or click the 'BP' column in CPU view to add breakpoints.", 0.6f, 0.65f, 0.7f, 1.25f);
        } else {
            for (int i = 0; i < g_dbg.bp_count; i++) {
                DebugBreakpoint *bp = &g_dbg.breakpoints[i];
                float cur_by = bp_y + (float)i * 24.0f;
                if (i % 2 == 1) dbg_draw_rect(0, cur_by, (float)g_dbg.win_w, 24, 0.14f, 0.14f, 0.16f, 1.0f);

                char num_str[16]; snprintf(num_str, sizeof(num_str), "%d", i + 1);
                dbg_draw_text(15, cur_by + 5, num_str, 0.8f, 0.8f, 0.8f, 1.2f);
                dbg_draw_text(50, cur_by + 5, bp->enabled ? "ENABLED" : "DISABLED", bp->enabled ? 0.3f : 0.6f, bp->enabled ? 0.9f : 0.6f, 0.4f, 1.2f);

                char pc_str[32]; snprintf(pc_str, sizeof(pc_str), "%04X", bp->pc);
                dbg_draw_text(140, cur_by + 5, pc_str, 0.45f, 0.85f, 0.55f, 1.2f);

                char l_str[32]; snprintf(l_str, sizeof(l_str), "Line %d", bp->line);
                dbg_draw_text(260, cur_by + 5, l_str, 0.95f, 0.75f, 0.3f, 1.2f);

                char h_str[32]; snprintf(h_str, sizeof(h_str), "%d hits", bp->hit_count);
                dbg_draw_text(380, cur_by + 5, h_str, 0.8f, 0.8f, 0.9f, 1.2f);
            }
        }
    } else if (g_dbg.active_tab == TAB_MEMORY) {
        /* GLOBALS & ENVIRONMENT MEMORY MAP */
        dbg_draw_rect(0, main_y, (float)g_dbg.win_w, 22, 0.16f, 0.16f, 0.19f, 1.0f);
        dbg_draw_text(15, main_y + 6, "GLOBAL VARIABLE", 0.9f, 0.9f, 0.9f, 1.15f);
        dbg_draw_text(220, main_y + 6, "TYPE", 0.9f, 0.9f, 0.9f, 1.15f);
        dbg_draw_text(340, main_y + 6, "CURRENT VALUE / HEAP ADDRESS", 0.9f, 0.9f, 0.9f, 1.15f);

        float mem_y = main_y + 24.0f;
        lua_pushglobaltable(L);
        int row = 0;
        lua_pushnil(L);
        while (lua_next(L, -2) != 0 && row < 28) {
            float cur_my = mem_y + (float)row * 20.0f;
            if (row % 2 == 1) dbg_draw_rect(0, cur_my, (float)g_dbg.win_w, 20, 0.13f, 0.13f, 0.15f, 1.0f);

            const char *key_name = lua_tostring(L, -2);
            if (!key_name) key_name = "(non-string key)";
            const char *type_name = lua_typename(L, lua_type(L, -1));

            char val_repr[128];
            if (lua_isstring(L, -1)) {
                snprintf(val_repr, sizeof(val_repr), "\"%.32s\"", lua_tostring(L, -1));
            } else if (lua_isnumber(L, -1)) {
                snprintf(val_repr, sizeof(val_repr), "%s", lua_tostring(L, -1));
            } else if (lua_isboolean(L, -1)) {
                snprintf(val_repr, sizeof(val_repr), "%s", lua_toboolean(L, -1) ? "true" : "false");
            } else {
                snprintf(val_repr, sizeof(val_repr), "%s: 0x%p", type_name, lua_topointer(L, -1));
            }

            dbg_draw_text(15, cur_my + 4, key_name, 0.95f, 0.8f, 0.4f, 1.15f);
            dbg_draw_text(220, cur_my + 4, type_name, 0.5f, 0.8f, 0.6f, 1.15f);
            dbg_draw_text(340, cur_my + 4, val_repr, 0.85f, 0.9f, 0.95f, 1.15f);

            lua_pop(L, 1);
            row++;
        }
        lua_pop(L, 1);
    } else if (g_dbg.active_tab == TAB_SCRIPT) {
        /* SOURCE SCRIPT VIEW */
        dbg_draw_rect(0, main_y, (float)g_dbg.win_w, 22, 0.16f, 0.16f, 0.19f, 1.0f);
        dbg_draw_text(15, main_y + 6, "LINE", 0.9f, 0.9f, 0.9f, 1.15f);
        dbg_draw_text(60, main_y + 6, "PTR", 0.9f, 0.9f, 0.9f, 1.15f);
        char src_hdr[256];
        snprintf(src_hdr, sizeof(src_hdr), "SOURCE CODE: %s", g_dbg.script_src.file_path);
        dbg_draw_text(100, main_y + 6, src_hdr, 0.9f, 0.9f, 0.9f, 1.15f);

        float src_y = main_y + 24.0f;
        int active_line = (g_dbg.current_proto) ? luaG_getfuncline(g_dbg.current_proto, g_dbg.current_pc) : 0;
        int visible_src = (int)((main_h - 26) / 20.0f);

        /* Auto-scroll script to current line */
        if (active_line > 0) {
            if (active_line < g_dbg.src_scroll + 1) g_dbg.src_scroll = active_line - 1;
            else if (active_line > g_dbg.src_scroll + visible_src) g_dbg.src_scroll = active_line - visible_src / 2;
            if (g_dbg.src_scroll < 0) g_dbg.src_scroll = 0;
        }

        for (int i = 0; i < visible_src; i++) {
            int line_idx = g_dbg.src_scroll + i;
            if (line_idx >= g_dbg.script_src.line_count) break;
            float cur_sy = src_y + (float)i * 20.0f;
            int line_num = line_idx + 1;
            int is_active = (line_num == active_line);

            if (is_active) {
                dbg_draw_rect(0, cur_sy, (float)g_dbg.win_w, 20, 0.15f, 0.35f, 0.2f, 1.0f);
                dbg_draw_triangle(60.0f, cur_sy + 4.0f, 60.0f, cur_sy + 16.0f, 72.0f, cur_sy + 10.0f, 0.2f, 1.0f, 0.4f, 1.0f);
            } else if (i % 2 == 1) {
                dbg_draw_rect(0, cur_sy, (float)g_dbg.win_w, 20, 0.13f, 0.13f, 0.15f, 1.0f);
            }

            char num_str[16]; snprintf(num_str, sizeof(num_str), "%4d", line_num);
            dbg_draw_text(15, cur_sy + 4, num_str, 0.5f, 0.55f, 0.6f, 1.15f);

            const char *src_text = g_dbg.script_src.lines[line_idx];
            if (src_text) {
                dbg_draw_text(100, cur_sy + 4, src_text, is_active ? 1.0f : 0.85f, is_active ? 1.0f : 0.85f, is_active ? 0.6f : 0.9f, 1.2f);
            }
        }
    }

    /* --------------------------------------------------------------------- */
    /* 5. BOTTOM STATUS BAR (Y: win_h - 24 to win_h)                         */
    /* --------------------------------------------------------------------- */
    float btm_y = (float)g_dbg.win_h - bottom_bar_h;
    dbg_draw_rect(0, btm_y, (float)g_dbg.win_w, bottom_bar_h, 0.0f, 0.48f, 0.8f, 1.0f);

    const char *help_text = "F5/Space: Run/Pause | F7/F11: Step In | F8/F10: Step Over | F2/F9: BP | F6: Speed | P: Pause";
    dbg_draw_text(12, btm_y + 6, help_text, 1.0f, 1.0f, 1.0f, 1.15f);

    char exec_stat[128];
    snprintf(exec_stat, sizeof(exec_stat), "Instructions: %lld | Breakpoints: %d", g_dbg.instructions_executed, g_dbg.bp_count);
    dbg_draw_text((float)g_dbg.win_w - 280, btm_y + 6, exec_stat, 0.9f, 0.95f, 1.0f, 1.15f);

    g_dbg.mouse_left_pressed = 0;
}

/* ========================================================================= */
/* GLFW Event Callbacks                                                      */
/* ========================================================================= */

static void dbg_key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
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
            if (g_dbg.selected_pc > 0) g_dbg.selected_pc--;
            break;
        case GLFW_KEY_DOWN:
            if (g_dbg.selected_pc < g_dbg.disasm_count - 1) g_dbg.selected_pc++;
            break;
        case GLFW_KEY_PAGE_UP:
            g_dbg.disasm_scroll_pc = (g_dbg.disasm_scroll_pc > 10) ? g_dbg.disasm_scroll_pc - 10 : 0;
            break;
        case GLFW_KEY_PAGE_DOWN:
            g_dbg.disasm_scroll_pc += 10;
            break;
    }
}

static void dbg_mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
    (void)window; (void)mods;
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            g_dbg.mouse_left_down = 1;
            g_dbg.mouse_left_pressed = 1;
        } else if (action == GLFW_RELEASE) {
            g_dbg.mouse_left_down = 0;
        }
    }
}

static void dbg_cursor_pos_callback(GLFWwindow *window, double xpos, double ypos) {
    (void)window;
    g_dbg.mouse_x = xpos;
    g_dbg.mouse_y = ypos;
}

static void dbg_scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
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

static void dbg_window_size_callback(GLFWwindow *window, int width, int height) {
    (void)window;
    g_dbg.win_w = width;
    g_dbg.win_h = height;
}

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

static void poll_global_hotkeys(lua_State *L) {
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
            if (g_dbg.selected_pc > 0) g_dbg.selected_pc--;
        }
        if (is_key_just_pressed(VK_DOWN, &g_hotkeys.down_down)) {
            if (g_dbg.selected_pc < g_dbg.disasm_count - 1) g_dbg.selected_pc++;
        }
        if (is_key_just_pressed(VK_PRIOR, &g_hotkeys.pgup_down)) {
            g_dbg.disasm_scroll_pc = (g_dbg.disasm_scroll_pc > 10) ? g_dbg.disasm_scroll_pc - 10 : 0;
        }
        if (is_key_just_pressed(VK_NEXT, &g_hotkeys.pgdn_down)) {
            g_dbg.disasm_scroll_pc += 10;
        }
    }
}
#endif

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
#ifdef _WIN32
            poll_global_hotkeys(L);
#endif
            render_debugger_gui(L);
            glfwSwapBuffers(g_dbg.window);

            if (g_dbg.execution_delay_ms > 0) {
#ifdef _WIN32
                Sleep((DWORD)g_dbg.execution_delay_ms);
#endif
            }
        } else if ((g_dbg.instructions_executed & 127) == 0) {
            glfwPollEvents();
#ifdef _WIN32
            poll_global_hotkeys(L);
#endif
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
#ifdef _WIN32
        poll_global_hotkeys(L);
#endif
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

    g_dbg.window = window;
    g_dbg.win_w = 1280;
    g_dbg.win_h = 768;
    g_dbg.status = DBG_STATUS_PAUSED;
    g_dbg.active_tab = TAB_CPU;
    g_dbg.should_exit = 0;
    g_dbg.instructions_executed = 0;
    g_dbg.start_time = glfwGetTime();
    g_dbg.last_ui_update = glfwGetTime();
    strncpy(g_dbg.script_path, script_path, sizeof(g_dbg.script_path) - 1);

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

    glfwDestroyWindow(window);
    glfwTerminate();
    return (run_status == LUA_OK) ? 1 : 0;
}
