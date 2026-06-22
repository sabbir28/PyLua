/*
** PyLua Parser - Expressions
*/

#define lparser_c
#define LUA_CORE

#include "../../../include/lprefix.h"
#include <limits.h>
#include "../../../include/lua.h"
#include "../../../include/lcode.h"
#include "../../../include/llex.h"
#include "../../../include/lparser.h"
#include "lparser_priv.h"

typedef struct ConsControl {
  expdesc v;
  expdesc *t;
  int nh;
  int na;
  int tostore;
  int maxtostore;
} ConsControl;

#define MAX_CNST	(INT_MAX/2)

static void subexpr (LexState *ls, expdesc *v, int limit);

static void recfield (LexState *ls, ConsControl *cc) {
  FuncState *fs = ls->fs;
  lu_byte reg = ls->fs->freereg;
  expdesc tab, key, val;
  if (ls->t.token == TK_NAME) codename(ls, &key);
  else yindex(ls, &key);
  cc->nh++;
  checknext(ls, '=');
  tab = *cc->t;
  luaK_indexed(fs, &tab, &key);
  expr(ls, &val);
  luaK_storevar(fs, &tab, &val);
  fs->freereg = reg;
}

static void closelistfield (FuncState *fs, ConsControl *cc) {
  lua_assert(cc->tostore > 0);
  luaK_exp2nextreg(fs, &cc->v);
  cc->v.k = VVOID;
  if (cc->tostore >= cc->maxtostore) {
    luaK_setlist(fs, cc->t->u.info, cc->na, cc->tostore);
    cc->na += cc->tostore;
    cc->tostore = 0;
  }
}

static void lastlistfield (FuncState *fs, ConsControl *cc) {
  if (cc->tostore == 0) return;
  if (cc->v.k == VCALL || cc->v.k == VVARARG) {
    luaK_setmultret(fs, &cc->v);
    luaK_setlist(fs, cc->t->u.info, cc->na, LUA_MULTRET);
    cc->na--;
  }
  else {
    if (cc->v.k != VVOID) luaK_exp2nextreg(fs, &cc->v);
    luaK_setlist(fs, cc->t->u.info, cc->na, cc->tostore);
  }
  cc->na += cc->tostore;
}

static void listfield (LexState *ls, ConsControl *cc) {
  expr(ls, &cc->v);
  cc->tostore++;
}

static void field (LexState *ls, ConsControl *cc) {
  switch(ls->t.token) {
    case TK_NAME: {
      if (luaX_lookahead(ls) != '=') listfield(ls, cc);
      else recfield(ls, cc);
      break;
    }
    case '[': recfield(ls, cc); break;
    default: listfield(ls, cc); break;
  }
}

static int maxtostore (FuncState *fs) {
  int numfreeregs = MAX_FSTACK - fs->freereg;
  if (numfreeregs >= 160) return numfreeregs / 5;
  else if (numfreeregs >= 80) return 10;
  else return 1;
}

void constructor (LexState *ls, expdesc *t) {
  FuncState *fs = ls->fs;
  int line = ls->linenumber;
  int pc = luaK_codevABCk(fs, OP_NEWTABLE, 0, 0, 0, 0);
  ConsControl cc;
  luaK_code(fs, 0);
  cc.na = cc.nh = cc.tostore = 0;
  cc.t = t;
  init_exp(t, VNONRELOC, fs->freereg);
  luaK_reserveregs(fs, 1);
  init_exp(&cc.v, VVOID, 0);
  checknext(ls, '{');
  cc.maxtostore = maxtostore(fs);
  do {
    if (ls->t.token == '}') break;
    if (cc.v.k != VVOID) closelistfield(fs, &cc);
    field(ls, &cc);
    luaY_checklimit(fs, cc.tostore + cc.na + cc.nh, MAX_CNST, "items in a constructor");
  } while (testnext(ls, ',') || testnext(ls, ';'));
  check_match(ls, '}', '{', line);
  lastlistfield(fs, &cc);
  luaK_settablesize(fs, pc, t->u.info, cc.na, cc.nh);
}

void fieldsel (LexState *ls, expdesc *v) {
  FuncState *fs = ls->fs;
  expdesc key;
  luaK_exp2anyregup(fs, v);
  luaX_next(ls);
  codename(ls, &key);
  luaK_indexed(fs, v, &key);
}

void yindex (LexState *ls, expdesc *v) {
  luaX_next(ls);
  expr(ls, v);
  luaK_exp2val(ls->fs, v);
  checknext(ls, ']');
}

void funcargs (LexState *ls, expdesc *f) {
  FuncState *fs = ls->fs;
  expdesc args;
  int base, nparams;
  int line = ls->linenumber;
  switch (ls->t.token) {
    case '(': {
      luaX_next(ls);
      if (ls->t.token == ')') args.k = VVOID;
      else {
        explist(ls, &args);
        if (args.k == VCALL || args.k == VVARARG) luaK_setmultret(fs, &args);
      }
      check_match(ls, ')', '(', line);
      break;
    }
    case '{': constructor(ls, &args); break;
    case TK_STRING: {
      codestring(&args, ls->t.seminfo.ts);
      luaX_next(ls);
      break;
    }
    default: luaX_syntaxerror(ls, "function arguments expected");
  }
  base = f->u.info;
  if (args.k == VCALL || args.k == VVARARG) nparams = LUA_MULTRET;
  else {
    if (args.k != VVOID) luaK_exp2nextreg(fs, &args);
    nparams = fs->freereg - (base+1);
  }
  init_exp(f, VCALL, luaK_codeABC(fs, OP_CALL, base, nparams+1, 2));
  luaK_fixline(fs, line);
  fs->freereg = cast_byte(base + 1);
}

void primaryexp (LexState *ls, expdesc *v) {
  switch (ls->t.token) {
    case '(': {
      int line = ls->linenumber;
      luaX_next(ls);
      expr(ls, v);
      check_match(ls, ')', '(', line);
      luaK_dischargevars(ls->fs, v);
      return;
    }
    case TK_NAME: singlevar(ls, v); return;
    default: luaX_syntaxerror(ls, "unexpected symbol");
  }
}

void suffixedexp (LexState *ls, expdesc *v) {
  FuncState *fs = ls->fs;
  primaryexp(ls, v);
  for (;;) {
    switch (ls->t.token) {
      case '.': fieldsel(ls, v); break;
      case '[': {
        expdesc key;
        luaK_exp2anyregup(fs, v);
        yindex(ls, &key);
        luaK_indexed(fs, v, &key);
        break;
      }
      case ':': {
        expdesc key;
        luaX_next(ls);
        codename(ls, &key);
        luaK_self(fs, v, &key);
        funcargs(ls, v);
        break;
      }
      case '(': case TK_STRING: case '{': {
        luaK_exp2nextreg(fs, v);
        funcargs(ls, v);
        break;
      }
      default: return;
    }
  }
}

void simpleexp (LexState *ls, expdesc *v) {
  switch (ls->t.token) {
    case TK_FLT: { init_exp(v, VKFLT, 0); v->u.nval = ls->t.seminfo.r; break; }
    case TK_INT: { init_exp(v, VKINT, 0); v->u.ival = ls->t.seminfo.i; break; }
    case TK_STRING: { codestring(v, ls->t.seminfo.ts); break; }
    case TK_NIL: { init_exp(v, VNIL, 0); break; }
    case TK_TRUE: { init_exp(v, VTRUE, 0); break; }
    case TK_FALSE: { init_exp(v, VFALSE, 0); break; }
    case TK_DOTS: {
      FuncState *fs = ls->fs;
      check_condition(ls, isvararg(fs->f), "cannot use '...' outside a vararg function");
      init_exp(v, VVARARG, luaK_codeABC(fs, OP_VARARG, 0, fs->f->numparams, 1));
      break;
    }
    case '{': constructor(ls, v); return;
    case TK_FUNCTION: {
      luaX_next(ls);
      body(ls, v, 0, ls->linenumber);
      return;
    }
    default: suffixedexp(ls, v); return;
  }
  luaX_next(ls);
}

static UnOpr getunopr (int op) {
  switch (op) {
    case TK_NOT: return OPR_NOT;
    case '-': return OPR_MINUS;
    case '~': return OPR_BNOT;
    case '#': return OPR_LEN;
    default: return OPR_NOUNOPR;
  }
}

static BinOpr getbinopr (int op) {
  switch (op) {
    case '+': return OPR_ADD; case '-': return OPR_SUB; case '*': return OPR_MUL;
    case '%': return OPR_MOD; case '^': return OPR_POW; case '/': return OPR_DIV;
    case TK_IDIV: return OPR_IDIV; case '&': return OPR_BAND; case '|': return OPR_BOR;
    case '~': return OPR_BXOR; case TK_SHL: return OPR_SHL; case TK_SHR: return OPR_SHR;
    case TK_CONCAT: return OPR_CONCAT; case TK_NE: return OPR_NE; case TK_EQ: return OPR_EQ;
    case '<': return OPR_LT; case TK_LE: return OPR_LE; case '>': return OPR_GT;
    case TK_GE: return OPR_GE; case TK_AND: return OPR_AND; case TK_OR: return OPR_OR;
    default: return OPR_NOBINOPR;
  }
}

static const struct { lu_byte left; lu_byte right; } priority[] = {
   {10, 10}, {10, 10}, {11, 11}, {11, 11}, {14, 13}, {11, 11}, {11, 11},
   {6, 6}, {4, 4}, {5, 5}, {7, 7}, {7, 7}, {9, 8}, {3, 3}, {3, 3}, {3, 3},
   {3, 3}, {3, 3}, {3, 3}, {2, 2}, {1, 1}
};

#define UNARY_PRIORITY	12

static void subexpr (LexState *ls, expdesc *v, int limit) {
  BinOpr op;
  UnOpr uop;
  luaE_incCstack(ls->L);
  uop = getunopr(ls->t.token);
  if (uop != OPR_NOUNOPR) {
    int line = ls->linenumber;
    luaX_next(ls);
    subexpr(ls, v, UNARY_PRIORITY);
    luaK_prefix(ls->fs, uop, v, line);
  }
  else simpleexp(ls, v);
  op = getbinopr(ls->t.token);
  while (op != OPR_NOBINOPR && priority[op].left > limit) {
    expdesc v2;
    BinOpr nextop;
    int line = ls->linenumber;
    luaX_next(ls);
    luaK_infix(ls->fs, op, v);
    nextop = subexpr(ls, &v2, priority[op].right);
    luaK_posfix(ls->fs, op, v, &v2, line);
    op = nextop;
  }
  ls->L->nCcalls--;
}

void expr (LexState *ls, expdesc *v) {
  subexpr(ls, v, 0);
}

int explist (LexState *ls, expdesc *v) {
  int n = 1;
  expr(ls, v);
  while (testnext(ls, ',')) {
    luaK_exp2nextreg(ls->fs, v);
    expr(ls, v);
    n++;
  }
  return n;
}
