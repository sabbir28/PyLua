/*
** PyLua Parser - Control Structures
*/

#define lparser_c
#define LUA_CORE

#include "../../../include/lprefix.h"
#include "../../../include/lua.h"
#include "../../../include/lcode.h"
#include "../../../include/llex.h"
#include "../../../include/lparser.h"
#include "lparser_priv.h"

static void exp1 (LexState *ls) {
  expdesc e;
  expr(ls, &e);
  luaK_exp2nextreg(ls->fs, &e);
  lua_assert(e.k == VNONRELOC);
}

static void fixforjump (FuncState *fs, int pc, int dest, int back) {
  Instruction *jmp = &fs->f->code[pc];
  int offset = dest - (pc + 1);
  if (back) offset = -offset;
  if (l_unlikely(offset > MAXARG_Bx)) luaX_syntaxerror(fs->ls, "control structure too long");
  SETARG_Bx(*jmp, offset);
}

int cond (LexState *ls) {
  expdesc v;
  int has_paren = testnext(ls, '(');
  expr(ls, &v);
  if (has_paren) checknext(ls, ')');
  if (v.k == VNIL) v.k = VFALSE;
  luaK_goiftrue(ls->fs, &v);
  return v.f;
}

void gotostat (LexState *ls, int line) {
  newgotoentry(ls, str_checkname(ls), line);
}

void breakstat (LexState *ls, int line) {
  BlockCnt *bl;
  for (bl = ls->fs->bl; bl != NULL; bl = bl->previous) {
    if (bl->isloop) goto ok;
  }
  luaX_syntaxerror(ls, "break outside loop");
 ok:
  bl->isloop = 2;
  luaX_next(ls);
  newgotoentry(ls, ls->brkn, line);
}

static void checkrepeated (LexState *ls, TString *name) {
  Labeldesc *lb = findlabel(ls, name, ls->fs->firstlabel);
  if (l_unlikely(lb != NULL))
    luaK_semerror(ls, "label '%s' already defined on line %d", getstr(name), lb->line);
}

void labelstat (LexState *ls, TString *name, int line) {
  checknext(ls, TK_DBCOLON);
  while (ls->t.token == ';' || ls->t.token == TK_DBCOLON) statement(ls);
  checkrepeated(ls, name);
  createlabel(ls, name, line, block_follow(ls, 0));
}

void whilestat (LexState *ls, int line) {
  FuncState *fs = ls->fs;
  int whileinit;
  int condexit;
  BlockCnt bl;
  luaX_next(ls);
  whileinit = luaK_getlabel(fs);
  condexit = cond(ls);
  enterblock(fs, &bl, 1);
  checknext(ls, TK_DO);
  block(ls);
  luaK_jumpto(fs, whileinit);
  check_match(ls, TK_END, TK_WHILE, line);
  leaveblock(fs);
  luaK_patchtohere(fs, condexit);
}

void repeatstat (LexState *ls, int line) {
  int condexit;
  FuncState *fs = ls->fs;
  int repeat_init = luaK_getlabel(fs);
  BlockCnt bl1, bl2;
  enterblock(fs, &bl1, 1);
  enterblock(fs, &bl2, 0);
  luaX_next(ls);
  statlist(ls);
  check_match(ls, TK_UNTIL, TK_REPEAT, line);
  condexit = cond(ls);
  leaveblock(fs);
  if (bl2.upval) {
    int exit = luaK_jump(fs);
    luaK_patchtohere(fs, condexit);
    luaK_codeABC(fs, OP_CLOSE, reglevel(fs, bl2.nactvar), 0, 0);
    condexit = luaK_jump(fs);
    luaK_patchtohere(fs, exit);
  }
  luaK_patchlist(fs, condexit, repeat_init);
  leaveblock(fs);
}

static void forbody (LexState *ls, int base, int line, int nvars, int isgen) {
  static const OpCode forprep[2] = {OP_FORPREP, OP_TFORPREP};
  static const OpCode forloop[2] = {OP_FORLOOP, OP_TFORLOOP};
  BlockCnt bl;
  FuncState *fs = ls->fs;
  int prep, endfor;
  checknext(ls, TK_DO);
  prep = luaK_codeABx(fs, forprep[isgen], base, 0);
  fs->freereg--;
  enterblock(fs, &bl, 0);
  adjustlocalvars(ls, nvars);
  luaK_reserveregs(fs, nvars);
  block(ls);
  leaveblock(fs);
  fixforjump(fs, prep, luaK_getlabel(fs), 0);
  if (isgen) {
    luaK_codeABC(fs, OP_TFORCALL, base, 0, nvars);
    luaK_fixline(fs, line);
  }
  endfor = luaK_codeABx(fs, forloop[isgen], base, 0);
  fixforjump(fs, endfor, prep + 1, 1);
  luaK_fixline(fs, line);
}

#define LOOPVARKIND	RDKCONST

static void fornum (LexState *ls, TString *varname, int line) {
  FuncState *fs = ls->fs;
  int base = fs->freereg;
  new_localvarliteral(ls, "(for state)");
  new_localvarliteral(ls, "(for state)");
  new_localvarliteral(ls, "(for state)");
  new_varkind(ls, varname, LOOPVARKIND);
  checknext(ls, '=');
  exp1(ls);
  checknext(ls, ',');
  exp1(ls);
  if (testnext(ls, ',')) exp1(ls);
  else { luaK_int(fs, fs->freereg, 1); luaK_reserveregs(fs, 1); }
  adjustlocalvars(ls, 3);
  forbody(ls, base, line, 1, 0);
}

static void forlist (LexState *ls, TString *indexname) {
  FuncState *fs = ls->fs;
  expdesc e;
  int nvars = 4;
  int line;
  int base = fs->freereg;
  new_localvarliteral(ls, "(for state)");
  new_localvarliteral(ls, "(for state)");
  new_localvarliteral(ls, "(for state)");
  new_varkind(ls, indexname, LOOPVARKIND);
  while (testnext(ls, ',')) { new_localvar(ls, str_checkname(ls)); nvars++; }
  checknext(ls, TK_IN);
  line = ls->linenumber;
  adjust_assign(ls, 4, explist(ls, &e), &e);
  adjustlocalvars(ls, 3);
  marktobeclosed(fs);
  luaK_checkstack(fs, 2);
  forbody(ls, base, line, nvars - 3, 1);
}

void forstat (LexState *ls, int line) {
  FuncState *fs = ls->fs;
  TString *varname;
  BlockCnt bl;
  enterblock(fs, &bl, 1);
  luaX_next(ls);
  varname = str_checkname(ls);
  switch (ls->t.token) {
    case '=': fornum(ls, varname, line); break;
    case ',': case TK_IN: forlist(ls, varname); break;
    default: luaX_syntaxerror(ls, "'=' or 'in' expected");
  }
  check_match(ls, TK_END, TK_FOR, line);
  leaveblock(fs);
}

void test_then_block (LexState *ls, int *escapelist) {
  FuncState *fs = ls->fs;
  int condtrue;
  int has_brace = 0;
  luaX_next(ls);
  condtrue = cond(ls);
  if (ls->t.token == '{') { has_brace = 1; luaX_next(ls); }
  else checknext(ls, TK_THEN);
  block(ls);
  if (has_brace) checknext(ls, '}');
  if (ls->t.token == TK_ELSE || ls->t.token == TK_ELSEIF)
    luaK_concat(fs, escapelist, luaK_jump(fs));
  luaK_patchtohere(fs, condtrue);
}

void ifstat (LexState *ls, int line) {
  FuncState *fs = ls->fs;
  int escapelist = NO_JUMP;
  test_then_block(ls, &escapelist);
  while (ls->t.token == TK_ELSEIF) test_then_block(ls, &escapelist);
  if (testnext(ls, TK_ELSE)) {
    if (ls->t.token == '{') { luaX_next(ls); block(ls); checknext(ls, '}'); }
    else block(ls);
  }
  if (ls->t.token == TK_END) luaX_next(ls);
  luaK_patchtohere(fs, escapelist);
}
