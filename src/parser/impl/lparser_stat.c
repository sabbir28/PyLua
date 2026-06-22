/*
** PyLua Parser - Statements
*/

#define lparser_c
#define LUA_CORE

#include "../../../include/lprefix.h"
#include <string.h>
#include "../../../include/lua.h"
#include "../../../include/lcode.h"
#include "../../../include/llex.h"
#include "../../../include/lparser.h"
#include "../../../include/lstate.h"
#include "../../../include/lstring.h"
#include "lparser_priv.h"

struct LHS_assign {
  struct LHS_assign *prev;
  expdesc v;
};

static void check_conflict (LexState *ls, struct LHS_assign *lh, expdesc *v) {
  FuncState *fs = ls->fs;
  lu_byte extra = fs->freereg;
  int conflict = 0;
  for (; lh; lh = lh->prev) {
    if (vkisindexed(lh->v.k)) {
      if (lh->v.k == VINDEXUP) {
        if (v->k == VUPVAL && lh->v.u.ind.t == v->u.info) {
          conflict = 1; lh->v.k = VINDEXSTR; lh->v.u.ind.t = extra;
        }
      } else {
        if (v->k == VLOCAL && lh->v.u.ind.t == v->u.var.ridx) {
          conflict = 1; lh->v.u.ind.t = extra;
        }
        if (lh->v.k == VINDEXED && v->k == VLOCAL && lh->v.u.ind.idx == v->u.var.ridx) {
          conflict = 1; lh->v.u.ind.idx = extra;
        }
      }
    }
  }
  if (conflict) {
    if (v->k == VLOCAL) luaK_codeABC(fs, OP_MOVE, extra, v->u.var.ridx, 0);
    else luaK_codeABC(fs, OP_GETUPVAL, extra, v->u.info, 0);
    luaK_reserveregs(fs, 1);
  }
}

static void storevartop (FuncState *fs, expdesc *var) {
  expdesc e;
  init_exp(&e, VNONRELOC, fs->freereg - 1);
  luaK_storevar(fs, var, &e);
}

static void restassign (LexState *ls, struct LHS_assign *lh, int nvars) {
  expdesc e;
  check_condition(ls, vkisvar(lh->v.k), "syntax error");
  check_readonly(ls, &lh->v);
  if (testnext(ls, ',')) {
    struct LHS_assign nv;
    nv.prev = lh;
    suffixedexp(ls, &nv.v);
    if (!vkisindexed(nv.v.k)) check_conflict(ls, lh, &nv.v);
    luaE_incCstack(ls->L);
    restassign(ls, &nv, nvars+1);
    ls->L->nCcalls--;
  } else {
    int nexps;
    checknext(ls, '=');
    nexps = explist(ls, &e);
    if (nexps != nvars) adjust_assign(ls, nvars, nexps, &e);
    else { luaK_setoneret(ls->fs, &e); luaK_storevar(ls->fs, &lh->v, &e); return; }
  }
  storevartop(ls->fs, &lh->v);
}

static void localfunc (LexState *ls) {
  expdesc b;
  FuncState *fs = ls->fs;
  int fvar = fs->nactvar;
  new_localvar(ls, str_checkname(ls));
  adjustlocalvars(ls, 1);
  body(ls, &b, 0, ls->linenumber);
  localdebuginfo(fs, fvar)->startpc = fs->pc;
}

static lu_byte getvarattribute (LexState *ls, lu_byte df) {
  if (testnext(ls, '<')) {
    TString *ts = str_checkname(ls);
    const char *attr = getstr(ts);
    checknext(ls, '>');
    if (strcmp(attr, "const") == 0) return RDKCONST;
    else if (strcmp(attr, "close") == 0) return RDKTOCLOSE;
    else luaK_semerror(ls, "unknown attribute '%s'", attr);
  }
  return df;
}

static void checktoclose (FuncState *fs, int level) {
  if (level != -1) {
    marktobeclosed(fs);
    luaK_codeABC(fs, OP_TBC, reglevel(fs, level), 0, 0);
  }
}

static void localstat (LexState *ls) {
  FuncState *fs = ls->fs;
  int toclose = -1;
  Vardesc *var;
  int vidx;
  int nvars = 0;
  int nexps;
  expdesc e;
  lu_byte defkind = getvarattribute(ls, VDKREG);
  do {
    TString *vname = str_checkname(ls);
    lu_byte kind = getvarattribute(ls, defkind);
    vidx = new_varkind(ls, vname, kind);
    if (kind == RDKTOCLOSE) {
      if (toclose != -1) luaK_semerror(ls, "multiple to-be-closed variables in local list");
      toclose = fs->nactvar + nvars;
    }
    nvars++;
  } while (testnext(ls, ','));
  if (testnext(ls, '=')) nexps = explist(ls, &e);
  else { e.k = VVOID; nexps = 0; }
  var = getlocalvardesc(fs, vidx);
  if (nvars == nexps && var->vd.kind == RDKCONST && luaK_exp2const(fs, &e, &var->k)) {
    var->vd.kind = RDKCTC;
    adjustlocalvars(ls, nvars - 1);
    fs->nactvar++;
  } else {
    adjust_assign(ls, nvars, nexps, &e);
    adjustlocalvars(ls, nvars);
  }
  checktoclose(fs, toclose);
}

static int funcname (LexState *ls, expdesc *v) {
  int ismethod = 0;
  singlevar(ls, v);
  while (ls->t.token == '.') fieldsel(ls, v);
  if (ls->t.token == ':') { ismethod = 1; fieldsel(ls, v); }
  return ismethod;
}

static void funcstat (LexState *ls, int line) {
  int ismethod;
  expdesc v, b;
  luaX_next(ls);
  ismethod = funcname(ls, &v);
  check_readonly(ls, &v);
  body(ls, &b, ismethod, line);
  luaK_storevar(ls->fs, &v, &b);
  luaK_fixline(ls->fs, line);
}

static void exprstat (LexState *ls) {
  /* stat -> func | assignment */
  FuncState *fs = ls->fs;
  struct LHS_assign v;

  /*
  ** PyLua Feature: Optional 'local' for newly assigned variables.
  ** If the statement starts with a NAME followed by '=' and the name
  ** is not currently a local variable or upvalue, we treat it as 
  ** a local declaration.
  */
  if (ls->t.token == TK_NAME && luaX_lookahead(ls) == '=') {
    TString *name = ls->t.seminfo.ts;
    expdesc e;
    singlevaraux(fs, name, &e, 1);
    if (e.k == VGLOBAL) {
      /* Not a local or upvalue, auto-local! */
      new_localvar(ls, str_checkname(ls));
      checknext(ls, '=');
      explist(ls, &e);
      adjust_assign(ls, 1, 1, &e);
      adjustlocalvars(ls, 1);
      return;
    }
  }

  suffixedexp(ls, &v.v);
  if (ls->t.token == '=' || ls->t.token == ',') { /* stat -> assignment ? */
    v.prev = NULL;
    restassign(ls, &v, 1);
  }
  else {  /* stat -> func */
    Instruction *inst;
    check_condition(ls, v.v.k == VCALL, "syntax error");
    inst = &getinstruction(fs, &v.v);
    SETARG_C(*inst, 1);  /* call statement uses no results */
  }
}

static void retstat (LexState *ls) {
  FuncState *fs = ls->fs;
  expdesc e;
  int nret;
  int first = luaY_nvarstack(fs);
  if (block_follow(ls, 1) || ls->t.token == ';') nret = 0;
  else {
    nret = explist(ls, &e);
    if (e.k == VCALL || e.k == VVARARG) {
      luaK_setmultret(fs, &e);
      if (e.k == VCALL && nret == 1 && !fs->bl->insidetbc) {
        SET_OPCODE(getinstruction(fs,&e), OP_TAILCALL);
        lua_assert(GETARG_A(getinstruction(fs,&e)) == luaY_nvarstack(fs));
      }
      nret = LUA_MULTRET;
    } else {
      if (nret == 1) first = luaK_exp2anyreg(fs, &e);
      else { luaK_exp2nextreg(fs, &e); lua_assert(nret == fs->freereg - first); }
    }
  }
  luaK_ret(fs, first, nret);
  testnext(ls, ';');
}

void setvararg (FuncState *fs) {
  fs->f->flag |= PF_VAHID;
  luaK_codeABC(fs, OP_VARARGPREP, 0, 0, 0);
}

void parlist (LexState *ls) {
  FuncState *fs = ls->fs;
  Proto *f = fs->f;
  int nparams = 0;
  int varargk = 0;
  if (ls->t.token != ')') {
    do {
      switch (ls->t.token) {
        case TK_NAME: new_localvar(ls, str_checkname(ls)); nparams++; break;
        case TK_DOTS:
          varargk = 1; luaX_next(ls);
          if (ls->t.token == TK_NAME) new_varkind(ls, str_checkname(ls), RDKVAVAR);
          else new_localvarliteral(ls, "(vararg table)");
          break;
        default: luaX_syntaxerror(ls, "<name> or '...' expected");
      }
    } while (!varargk && testnext(ls, ','));
  }
  adjustlocalvars(ls, nparams);
  f->numparams = cast_byte(fs->nactvar);
  if (varargk) { setvararg(fs); adjustlocalvars(ls, 1); }
  luaK_reserveregs(fs, fs->nactvar);
}

void body (LexState *ls, expdesc *e, int ismethod, int line) {
  FuncState new_fs;
  BlockCnt bl;
  new_fs.f = addprototype(ls);
  new_fs.f->linedefined = line;
  open_func(ls, &new_fs, &bl);
  checknext(ls, '(');
  if (ismethod) { new_localvarliteral(ls, "self"); adjustlocalvars(ls, 1); }
  parlist(ls);
  checknext(ls, ')');
  statlist(ls);
  new_fs.f->lastlinedefined = ls->linenumber;
  check_match(ls, TK_END, TK_FUNCTION, line);
  codeclosure(ls, e);
  close_func(ls);
}

void importstat (LexState *ls) {
  FuncState *fs = ls->fs;
  TString *name;
  expdesc f, arg, e;
  luaX_next(ls);
  name = str_checkname(ls);
  singlevaraux(fs, luaS_newliteral(ls->L, "require"), &f, 1);
  if (f.k == VGLOBAL) buildglobal(ls, luaS_newliteral(ls->L, "require"), &f);
  codestring(&arg, name);
  luaK_exp2nextreg(fs, &f);
  luaK_exp2nextreg(fs, &arg);
  luaK_codeABC(fs, OP_CALL, f.u.info, 2, 2);
  init_exp(&e, VNONRELOC, f.u.info);
  new_localvar(ls, name);
  adjust_assign(ls, 1, 1, &e);
  adjustlocalvars(ls, 1);
}

void block (LexState *ls) {
  FuncState *fs = ls->fs;
  BlockCnt bl;
  enterblock(fs, &bl, 0);
  statlist(ls);
  leaveblock(fs);
}

int block_follow (LexState *ls, int withuntil) {
  switch (ls->t.token) {
    case TK_ELSE: case TK_ELSEIF: case TK_END: case TK_EOS: case '}': return 1;
    case TK_UNTIL: return withuntil;
    default: return 0;
  }
}

void statlist (LexState *ls) {
  while (!block_follow(ls, 1)) {
    if (ls->t.token == TK_RETURN) { statement(ls); return; }
    statement(ls);
  }
}

void statement (LexState *ls) {
  int line = ls->linenumber;
  luaE_incCstack(ls->L);
  switch (ls->t.token) {
    case ';': luaX_next(ls); break;
    case TK_IF: ifstat(ls, line); break;
    case TK_WHILE: whilestat(ls, line); break;
    case TK_DO: luaX_next(ls); block(ls); check_match(ls, TK_END, TK_DO, line); break;
    case TK_FOR: forstat(ls, line); break;
    case TK_REPEAT: repeatstat(ls, line); break;
    case TK_FUNCTION: funcstat(ls, line); break;
    case TK_LOCAL:
      luaX_next(ls);
      if (testnext(ls, TK_FUNCTION)) localfunc(ls);
      else localstat(ls);
      break;
    case TK_RETURN: luaX_next(ls); retstat(ls); break;
    case TK_BREAK: breakstat(ls, line); break;
    case TK_GOTO: luaX_next(ls); gotostat(ls, line); break;
    case TK_IMPORT: importstat(ls); break;
    default: exprstat(ls); break;
  }
  ls->fs->freereg = luaY_nvarstack(ls->fs);
  ls->L->nCcalls--;
}
