/*
** PyLua Visual Bytecode / CPU Debugger PRO
** Module: Bytecode Disassembly & TValue Inspection
*/

#ifndef DEBUGGER_DISASM_H
#define DEBUGGER_DISASM_H

#include "Debugger_Types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Value Inspection */
void format_tvalue(char *buf, size_t sz, const TValue *v);

/* Bytecode Disassembly */
void decode_proto(Proto *p);
void free_disasm_lines(void);

#ifdef __cplusplus
}
#endif

#endif /* DEBUGGER_DISASM_H */

