/*
** PyLua Visual Bytecode / CPU Debugger PRO
** Module: Bytecode Disassembly & TValue Inspection Implementation
*/

#include "Debugger_Disasm.h"
#include "Debugger_Core.h"

/* ========================================================================= */
/* TValue Inspection Helpers                                                 */
/* ========================================================================= */

void format_tvalue(char *buf, size_t sz, const TValue *v) {
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

/* ========================================================================= */
/* Disassembly Decoder                                                       */
/* ========================================================================= */

void free_disasm_lines(void) {
    if (g_dbg.disasm_lines) {
        free(g_dbg.disasm_lines);
        g_dbg.disasm_lines = NULL;
        g_dbg.disasm_count = 0;
    }
}

void decode_proto(Proto *p) {
    free_disasm_lines();
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

