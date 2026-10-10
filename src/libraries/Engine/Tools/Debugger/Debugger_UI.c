/*
** PyLua Visual Bytecode / CPU Debugger PRO
** Module: Debugger UI & Window Layout Implementation
*/

#include "Debugger_UI.h"
#include "Debugger_Core.h"
#include "Debugger_Render.h"
#include "Debugger_Disasm.h"

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
/* Main Debugger GUI Drawing                                                 */
/* ========================================================================= */

void render_debugger_gui(lua_State *L) {
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
        float sb_w = 14.0f;
        float disasm_table_w = split_x - sb_w;

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
        int max_scroll = g_dbg.disasm_count - max_visible_rows;
        if (max_scroll < 0) max_scroll = 0;

        /* Clamp disasm_scroll_pc within valid bounds */
        if (g_dbg.disasm_scroll_pc > max_scroll) g_dbg.disasm_scroll_pc = max_scroll;
        if (g_dbg.disasm_scroll_pc < 0) g_dbg.disasm_scroll_pc = 0;

        for (int r = 0; r < max_visible_rows; r++) {
            int pc_idx = g_dbg.disasm_scroll_pc + r;
            if (pc_idx >= g_dbg.disasm_count) break;

            float cur_row_y = row_y_start + (float)r * row_h;
            DisasmLine *line = &g_dbg.disasm_lines[pc_idx];
            int is_cur = (pc_idx == g_dbg.current_pc);
            int is_sel = (pc_idx == g_dbg.selected_pc);
            int bp_idx = find_breakpoint(pc_idx);

            /* Zebra striping up to the scrollbar edge */
            if (is_cur) {
                dbg_draw_rect(0, cur_row_y, disasm_table_w, row_h, 0.12f, 0.28f, 0.45f, 1.0f);
                dbg_draw_rect_outline(0, cur_row_y, disasm_table_w, row_h, 1.0f, 0.0f, 0.65f, 1.0f, 1.0f);
            } else if (is_sel) {
                dbg_draw_rect(0, cur_row_y, disasm_table_w, row_h, 0.22f, 0.24f, 0.28f, 1.0f);
                dbg_draw_rect_outline(0, cur_row_y, disasm_table_w, row_h, 1.0f, 0.4f, 0.5f, 0.6f, 1.0f);
            } else if (r % 2 == 1) {
                dbg_draw_rect(0, cur_row_y, disasm_table_w, row_h, 0.13f, 0.13f, 0.15f, 1.0f);
            }

            /* Click handling for row selection and breakpoint toggle */
            if (g_dbg.mouse_left_pressed && g_dbg.mouse_y >= cur_row_y && g_dbg.mouse_y < cur_row_y + row_h && g_dbg.mouse_x < disasm_table_w) {
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

        /* ================================================================= */
        /* DISASSEMBLY VERTICAL SCROLLBAR                                     */
        /* ================================================================= */
        float sb_x = disasm_table_w;
        float sb_y = row_y_start;
        float sb_h = main_h - 22.0f;

        /* Track background */
        dbg_draw_rect(sb_x, sb_y, sb_w, sb_h, 0.12f, 0.12f, 0.14f, 1.0f);
        dbg_draw_line(sb_x, sb_y, sb_x, sb_y + sb_h, 1.0f, 0.22f, 0.22f, 0.26f, 1.0f);

        if (g_dbg.disasm_count > 0 && max_scroll > 0) {
            float ratio = (float)max_visible_rows / (float)g_dbg.disasm_count;
            float thumb_h = sb_h * ratio;
            if (thumb_h < 26.0f) thumb_h = 26.0f;
            if (thumb_h > sb_h - 4.0f) thumb_h = sb_h - 4.0f;
            float avail_h = sb_h - thumb_h;

            /* Mouse dragging interaction */
            if (g_dbg.disasm_sb_dragging) {
                if (g_dbg.mouse_left_down) {
                    float new_thumb_y = (float)g_dbg.mouse_y - g_dbg.disasm_sb_drag_offset_y;
                    float s_ratio = (new_thumb_y - sb_y) / avail_h;
                    if (s_ratio < 0.0f) s_ratio = 0.0f;
                    if (s_ratio > 1.0f) s_ratio = 1.0f;
                    g_dbg.disasm_scroll_pc = (int)(s_ratio * (float)max_scroll + 0.5f);
                } else {
                    g_dbg.disasm_sb_dragging = 0;
                }
            }

            /* Re-clamp disasm_scroll_pc */
            if (g_dbg.disasm_scroll_pc > max_scroll) g_dbg.disasm_scroll_pc = max_scroll;
            if (g_dbg.disasm_scroll_pc < 0) g_dbg.disasm_scroll_pc = 0;

            float scroll_ratio = (float)g_dbg.disasm_scroll_pc / (float)max_scroll;
            if (scroll_ratio < 0.0f) scroll_ratio = 0.0f;
            if (scroll_ratio > 1.0f) scroll_ratio = 1.0f;
            float thumb_y = sb_y + avail_h * scroll_ratio;

            /* Mouse click interaction on track or thumb */
            int mouse_in_sb = (g_dbg.mouse_x >= sb_x && g_dbg.mouse_x <= split_x &&
                               g_dbg.mouse_y >= sb_y && g_dbg.mouse_y <= sb_y + sb_h);
            if (mouse_in_sb && g_dbg.mouse_left_pressed) {
                if (g_dbg.mouse_y >= thumb_y && g_dbg.mouse_y <= thumb_y + thumb_h) {
                    g_dbg.disasm_sb_dragging = 1;
                    g_dbg.disasm_sb_drag_offset_y = (float)g_dbg.mouse_y - thumb_y;
                } else if (g_dbg.mouse_y < thumb_y) {
                    g_dbg.disasm_scroll_pc -= max_visible_rows;
                    if (g_dbg.disasm_scroll_pc < 0) g_dbg.disasm_scroll_pc = 0;
                } else {
                    g_dbg.disasm_scroll_pc += max_visible_rows;
                    if (g_dbg.disasm_scroll_pc > max_scroll) g_dbg.disasm_scroll_pc = max_scroll;
                }
            }

            int hover_thumb = (g_dbg.mouse_x >= sb_x && g_dbg.mouse_x <= split_x &&
                               g_dbg.mouse_y >= thumb_y && g_dbg.mouse_y <= thumb_y + thumb_h);
            float tr, tg, tb;
            if (g_dbg.disasm_sb_dragging) {
                /* Active Dragging: bright cyan */
                tr = 0.0f; tg = 0.65f; tb = 1.0f;
            } else if (hover_thumb) {
                /* Hover: bright slate */
                tr = 0.45f; tg = 0.48f; tb = 0.55f;
            } else {
                /* Normal: clean neutral slate */
                tr = 0.28f; tg = 0.29f; tb = 0.34f;
            }

            /* Draw thumb pill */
            dbg_draw_rect(sb_x + 2.0f, thumb_y + 1.0f, sb_w - 4.0f, thumb_h - 2.0f, tr, tg, tb, 1.0f);
            dbg_draw_rect_outline(sb_x + 2.0f, thumb_y + 1.0f, sb_w - 4.0f, thumb_h - 2.0f, 1.0f,
                                  tr * 1.2f, tg * 1.2f, tb * 1.2f, 1.0f);

            /* Decorative tactile grip lines in thumb center */
            if (thumb_h >= 32.0f) {
                float mid_y = thumb_y + thumb_h * 0.5f;
                float g_alpha = g_dbg.disasm_sb_dragging ? 0.95f : (hover_thumb ? 0.8f : 0.45f);
                dbg_draw_line(sb_x + 4.0f, mid_y - 3.0f, split_x - 4.0f, mid_y - 3.0f, 1.0f, 0.9f, 0.95f, 1.0f, g_alpha);
                dbg_draw_line(sb_x + 4.0f, mid_y,        split_x - 4.0f, mid_y,        1.0f, 0.9f, 0.95f, 1.0f, g_alpha);
                dbg_draw_line(sb_x + 4.0f, mid_y + 3.0f, split_x - 4.0f, mid_y + 3.0f, 1.0f, 0.9f, 0.95f, 1.0f, g_alpha);
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

