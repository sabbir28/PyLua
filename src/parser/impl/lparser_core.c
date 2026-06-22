/*
** PyLua Parser - Core
** Infrastructure, error handling, variable tracking, and entry points.
*/

#define lparser_c
#define LUA_CORE

#include "../../../include/lprefix.h"
#include <limits.h>
#include <string.h>
#include "../../../include/lua.h"
#include "../../../include/lcode.h"
#include "../../../include/ldebug.h"
#include "../../../include/ldo.h"
#include "../../../include/lfunc.h"
#include "../../../include/llex.h"
#include "../../../include/lmem.h"
#include "../../../include/lobject.h"
#include "../../../include/lopcodes.h"
#include "../../../include/lparser.h"
#include "../../../include/lstate.h"
#include "../../../include/lstring.h"
#include "../../../include/ltable.h"
#include "lparser_priv.h"

#define MAXVARS		200
#define hasmultret(k)		((k) == VCALL || (k) == VVARARG)
#define eqstr(a,b)	((a) == (b))

static l_noret error_expected (LexState *ls, int token) {
  luaX_syntaxerror(ls,
      luaO_pushfstring(ls->L, "%s expected", luaX_token2str(ls, token)));
}

static l_noret errorlimit (FuncState *fs, int limit, const char *what) {
  lua_State *L = fs->ls->L;
  const char *msg;
  int line = fs->f->linedefined;
  const char *where = (line == 0)
                      ? "main function"
                      : luaO_pushfstring(L, "function at line %d", line);
  msg = luaO_pushfstring(L, "too many %s (limit is %d) in %s",
                              what, limit, where);
  luaX_syntaxerror(fs->ls, msg);
}

void luaY_checklimit (FuncState *fs, int v, int l, const char *what) {
  if (l_unlikely(v > l)) errorlimit(fs, l, what);
}

int testnext (LexState *ls, int c) {
  if (ls->t.token == c) {
    luaX_next(ls);
    return 1;
  }
  else return 0;
}

void check (LexState *ls, int c) {
  if (ls->t.token != c)
    error_expected(ls, c);
}

void checknext (LexState *ls, int c) {
  check(ls, c);
  luaX_next(ls);
}

void check_match (LexState *ls, int what, int who, int where) {
  if (l_unlikely(!testnext(ls, what))) {
    if (where == ls->linenumber)
      error_expected(ls, what);
    else {
      luaX_syntaxerror(ls, luaO_pushfstring(ls->L,
             "%s expected (to close %s at line %d)",
              luaX_token2str(ls, what), luaX_token2str(ls, who), where));
    }
  }
}

TString *str_checkname (LexState *ls) {
  TString *ts;
  check(ls, TK_NAME);
  ts = ls->t.seminfo.ts;
  luaX_next(ls);
  return ts;
}

void init_exp (expdesc *e, expkind k, int i) {
  e->f = e->t = NO_JUMP;
  e->k = k;
  e->u.info = i;
}

void codestring (expdesc *e, TString *s) {
  e->f = e->t = NO_JUMP;
  e->k = VKSTR;
  e->u.strval = s;
}

void codename (LexState *ls, expdesc *e) {
  codestring(e, str_checkname(ls));
}

static short registerlocalvar (LexState *ls, FuncState *fs, TString *varname) {
  Proto *f = fs->f;
  int oldsize = f->sizelocvars;
  luaM_growvector(ls->L, f->locvars, fs->ndebugvars, f->sizelocvars,
                  LocVar, SHRT_MAX, "local variables");
  while (oldsize < f->sizelocvars)
    f->locvars[oldsize++].varname = NULL;
  f->locvars[fs->ndebugvars].varname = varname;
  f->locvars[fs->ndebugvars].startpc = fs->pc;
  luaC_objbarrier(ls->L, f, varname);
  return fs->ndebugvars++;
}

int new_varkind (LexState *ls, TString *name, lu_byte kind) {
  lua_State *L = ls->L;
  FuncState *fs = ls->fs;
  Dyndata *dyd = ls->dyd;
  Vardesc *var;
  luaM_growvector(L, dyd->actvar.arr, dyd->actvar.n + 1,
             dyd->actvar.size, Vardesc, SHRT_MAX, "variable declarations");
  var = &dyd->actvar.arr[dyd->actvar.n++];
  var->vd.kind = kind;
  var->vd.name = name;
  return dyd->actvar.n - 1 - fs->firstlocal;
}

int new_localvar (LexState *ls, TString *name) {
  return new_varkind(ls, name, VDKREG);
}

Vardesc *getlocalvardesc (FuncState *fs, int vidx) {
  return &fs->ls->dyd->actvar.arr[fs->firstlocal + vidx];
}

lu_byte reglevel (FuncState *fs, int nvar) {
  while (nvar-- > 0) {
    Vardesc *vd = getlocalvardesc(fs, nvar);
    if (varinreg(vd))
      return cast_byte(vd->vd.ridx + 1);
  }
  return 0;
}

lu_byte luaY_nvarstack (FuncState *fs) {
  return reglevel(fs, fs->nactvar);
}

LocVar *localdebuginfo (FuncState *fs, int vidx) {
  Vardesc *vd = getlocalvardesc(fs,  vidx);
  if (!varinreg(vd))
    return NULL;
  else {
    int idx = vd->vd.pidx;
    lua_assert(idx < fs->ndebugvars);
    return &fs->f->locvars[idx];
  }
}

void init_var (FuncState *fs, expdesc *e, int vidx) {
  e->f = e->t = NO_JUMP;
  e->k = VLOCAL;
  e->u.var.vidx = cast_short(vidx);
  e->u.var.ridx = getlocalvardesc(fs, vidx)->vd.ridx;
}

void check_readonly (LexState *ls, expdesc *e) {
  FuncState *fs = ls->fs;
  TString *varname = NULL;
  switch (e->k) {
    case VCONST: {
      varname = ls->dyd->actvar.arr[e->u.info].vd.name;
      break;
    }
    case VLOCAL: case VVARGVAR: {
      Vardesc *vardesc = getlocalvardesc(fs, e->u.var.vidx);
      if (vardesc->vd.kind != VDKREG)
        varname = vardesc->vd.name;
      break;
    }
    case VUPVAL: {
      Upvaldesc *up = &fs->f->upvalues[e->u.info];
      if (up->kind != VDKREG)
        varname = up->name;
      break;
    }
    case VVARGIND: {
      needvatab(fs->f);
      e->k = VINDEXED;
    }
    case VINDEXUP: case VINDEXSTR: case VINDEXED: {
      if (e->u.ind.ro)
        varname = tsvalue(&fs->f->k[e->u.ind.keystr]);
      break;
    }
    default:
      lua_assert(e->k == VINDEXI);
      return;
  }
  if (varname)
    luaK_semerror(ls, "attempt to assign to const variable '%s'",
                      getstr(varname));
}

void adjustlocalvars (LexState *ls, int nvars) {
  FuncState *fs = ls->fs;
  int rl = luaY_nvarstack(fs);
  int i;
  for (i = 0; i < nvars; i++) {
    int vidx = fs->nactvar++;
    Vardesc *var = getlocalvardesc(fs, vidx);
    var->vd.ridx = cast_byte(rl++);
    var->vd.pidx = registerlocalvar(ls, fs, var->vd.name);
    luaY_checklimit(fs, rl, MAXVARS, "local variables");
  }
}

void removevars (FuncState *fs, int tolevel) {
  fs->ls->dyd->actvar.n -= (fs->nactvar - tolevel);
  while (fs->nactvar > tolevel) {
    LocVar *var = localdebuginfo(fs, --fs->nactvar);
    if (var)
      var->endpc = fs->pc;
  }
}

int searchupvalue (FuncState *fs, TString *name) {
  int i;
  Upvaldesc *up = fs->f->upvalues;
  for (i = 0; i < fs->nups; i++) {
    if (eqstr(up[i].name, name)) return i;
  }
  return -1;
}

Upvaldesc *allocupvalue (FuncState *fs) {
  Proto *f = fs->f;
  int oldsize = f->sizeupvalues;
  luaY_checklimit(fs, fs->nups + 1, MAXUPVAL, "upvalues");
  luaM_growvector(fs->ls->L, f->upvalues, fs->nups, f->sizeupvalues,
                  Upvaldesc, MAXUPVAL, "upvalues");
  while (oldsize < f->sizeupvalues)
    f->upvalues[oldsize++].name = NULL;
  return &f->upvalues[fs->nups++];
}

int newupvalue (FuncState *fs, TString *name, expdesc *v) {
  Upvaldesc *up = allocupvalue(fs);
  FuncState *prev = fs->prev;
  if (v->k == VLOCAL) {
    up->instack = 1;
    up->idx = v->u.var.ridx;
    up->kind = getlocalvardesc(prev, v->u.var.vidx)->vd.kind;
    lua_assert(eqstr(name, getlocalvardesc(prev, v->u.var.vidx)->vd.name));
  }
  else {
    up->instack = 0;
    up->idx = cast_byte(v->u.info);
    up->kind = prev->f->upvalues[v->u.info].kind;
    lua_assert(eqstr(name, prev->f->upvalues[v->u.info].name));
  }
  up->name = name;
  luaC_objbarrier(fs->ls->L, fs->f, name);
  return fs->nups - 1;
}

int searchvar (FuncState *fs, TString *n, expdesc *var) {
  int i;
  for (i = cast_int(fs->nactvar) - 1; i >= 0; i--) {
    Vardesc *vd = getlocalvardesc(fs, i);
    if (varglobal(vd)) {
      if (vd->vd.name == NULL) {
        if (var->u.info < 0)
          var->u.info = fs->firstlocal + i;
      }
      else {
        if (eqstr(n, vd->vd.name)) {
          init_exp(var, VGLOBAL, fs->firstlocal + i);
          return VGLOBAL;
        }
        else if (var->u.info == -1)
          var->u.info = -2;
      }
    }
    else if (eqstr(n, vd->vd.name)) {
      if (vd->vd.kind == RDKCTC)
        init_exp(var, VCONST, fs->firstlocal + i);
      else {
        init_var(fs, var, i);
        if (vd->vd.kind == RDKVAVAR)
          var->k = VVARGVAR;
      }
      return cast_int(var->k);
    }
  }
  return -1;
}

void markupval (FuncState *fs, int level) {
  BlockCnt *bl = fs->bl;
  while (bl->nactvar > level)
    bl = bl->previous;
  bl->upval = 1;
  fs->needclose = 1;
}

void marktobeclosed (FuncState *fs) {
  BlockCnt *bl = fs->bl;
  bl->upval = 1;
  bl->insidetbc = 1;
  fs->needclose = 1;
}

void singlevaraux (FuncState *fs, TString *n, expdesc *var, int base) {
  int v = searchvar(fs, n, var);
  if (v >= 0) {
    if (!base) {
      if (var->k == VVARGVAR)
        luaK_vapar2local(fs, var);
      if (var->k == VLOCAL)
        markupval(fs, var->u.var.vidx);
    }
  }
  else {
    int idx = searchupvalue(fs, n);
    if (idx < 0) {
      if (fs->prev != NULL)
        singlevaraux(fs->prev, n, var, 0);
      if (var->k == VLOCAL || var->k == VUPVAL)
        idx  = newupvalue(fs, n, var);
      else
        return;
    }
    init_exp(var, VUPVAL, idx);
  }
}

void buildglobal (LexState *ls, TString *varname, expdesc *var) {
  FuncState *fs = ls->fs;
  expdesc key;
  init_exp(var, VGLOBAL, -1);
  singlevaraux(fs, ls->envn, var, 1);
  if (var->k == VGLOBAL)
    luaK_semerror(ls, "%s is global when accessing variable '%s'",
                       LUA_ENV, getstr(varname));
  luaK_exp2anyregup(fs, var);
  codestring(&key, varname);
  luaK_indexed(fs, var, &key);
}

void buildvar (LexState *ls, TString *varname, expdesc *var) {
  FuncState *fs = ls->fs;
  init_exp(var, VGLOBAL, -1);
  singlevaraux(fs, varname, var, 1);
  if (var->k == VGLOBAL) {
    int info = var->u.info;
    if (info == -2)
      luaK_semerror(ls, "variable '%s' not declared", getstr(varname));
    buildglobal(ls, varname, var);
    if (info != -1 && ls->dyd->actvar.arr[info].vd.kind == GDKCONST)
      var->u.ind.ro = 1;
    else
      lua_assert(info == -1 || ls->dyd->actvar.arr[info].vd.kind == GDKREG);
  }
}

void singlevar (LexState *ls, expdesc *var) {
  buildvar(ls, str_checkname(ls), var);
}

void adjust_assign (LexState *ls, int nvars, int nexps, expdesc *e) {
  FuncState *fs = ls->fs;
  int needed = nvars - nexps;
  luaK_checkstack(fs, needed);
  if (hasmultret(e->k)) {
    int extra = needed + 1;
    if (extra < 0) extra = 0;
    luaK_setreturns(fs, e, extra);
  }
  else {
    if (e->k != VVOID)
      luaK_exp2nextreg(fs, e);
    if (needed > 0)
      luaK_nil(fs, fs->freereg, needed);
  }
  if (needed > 0)
    luaK_reserveregs(fs, needed);
  else
    fs->freereg = cast_byte(fs->freereg + needed);
}

void enterblock (FuncState *fs, BlockCnt *bl, lu_byte isloop) {
  bl->isloop = isloop;
  bl->nactvar = fs->nactvar;
  bl->firstlabel = fs->ls->dyd->label.n;
  bl->firstgoto = fs->ls->dyd->gt.n;
  bl->upval = 0;
  bl->insidetbc = (fs->bl != NULL && fs->bl->insidetbc);
  bl->previous = fs->bl;
  fs->bl = bl;
  lua_assert(fs->freereg == luaY_nvarstack(fs));
}

void leaveblock (FuncState *fs) {
  BlockCnt *bl = fs->bl;
  LexState *ls = fs->ls;
  lu_byte stklevel = reglevel(fs, bl->nactvar);
  if (bl->previous && bl->upval)
    luaK_codeABC(fs, OP_CLOSE, stklevel, 0, 0);
  fs->freereg = stklevel;
  removevars(fs, bl->nactvar);
  lua_assert(bl->nactvar == fs->nactvar);
  if (bl->isloop == 2)
    createlabel(ls, ls->brkn, 0, 0);
  solvegotos(fs, bl);
  if (bl->previous == NULL) {
    if (bl->firstgoto < ls->dyd->gt.n) {
      Labeldesc *gt = &ls->dyd->gt.arr[bl->firstgoto];
      luaK_semerror(ls, "no visible label '%s' for <goto> at line %d",
                        getstr(gt->name), gt->line);
    }
  }
  fs->bl = bl->previous;
}

Labeldesc *findlabel (LexState *ls, TString *name, int ilb) {
  Dyndata *dyd = ls->dyd;
  for (; ilb < dyd->label.n; ilb++) {
    Labeldesc *lb = &dyd->label.arr[ilb];
    if (eqstr(lb->name, name)) return lb;
  }
  return NULL;
}

int newlabelentry (LexState *ls, Labellist *l, TString *name, int line, int pc) {
  int n = l->n;
  luaM_growvector(ls->L, l->arr, n, l->size, Labeldesc, SHRT_MAX, "labels/gotos");
  l->arr[n].name = name;
  l->arr[n].line = line;
  l->arr[n].nactvar = ls->fs->nactvar;
  l->arr[n].close = 0;
  l->arr[n].pc = pc;
  l->n = n + 1;
  return n;
}

int newgotoentry (LexState *ls, TString *name, int line) {
  FuncState *fs = ls->fs;
  int pc = luaK_jump(fs);
  luaK_codeABC(fs, OP_CLOSE, 0, 1, 0);
  return newlabelentry(ls, &ls->dyd->gt, name, line, pc);
}

void createlabel (LexState *ls, TString *name, int line, int last) {
  FuncState *fs = ls->fs;
  Labellist *ll = &ls->dyd->label;
  int l = newlabelentry(ls, ll, name, line, luaK_getlabel(fs));
  if (last) ll->arr[l].nactvar = fs->bl->nactvar;
}

void solvegotos (FuncState *fs, BlockCnt *bl) {
  LexState *ls = fs->ls;
  Labellist *gl = &ls->dyd->gt;
  int outlevel = reglevel(fs, bl->nactvar);
  int igt = bl->firstgoto;
  while (igt < gl->n) {
    Labeldesc *gt = &gl->arr[igt];
    Labeldesc *lb = findlabel(ls, gt->name, bl->firstlabel);
    if (lb != NULL) {
      if (l_unlikely(gt->nactvar < lb->nactvar)) {
        TString *tsname = getlocalvardesc(fs, gt->nactvar)->vd.name;
        const char *varname = (tsname != NULL) ? getstr(tsname) : "*";
        luaK_semerror(ls, "<goto %s> at line %d jumps into the scope of '%s'",
                          getstr(gt->name), gt->line, varname);
      }
      if (gt->close || (lb->nactvar < gt->nactvar && bl->upval)) {
        lu_byte stklevel = reglevel(fs, lb->nactvar);
        fs->f->code[gt->pc + 1] = fs->f->code[gt->pc];
        fs->f->code[gt->pc] = CREATE_ABCk(OP_CLOSE, stklevel, 0, 0, 0);
        gt->pc++;
      }
      luaK_patchlist(ls->fs, gt->pc, lb->pc);
      for (int i = igt; i < gl->n - 1; i++) gl->arr[i] = gl->arr[i + 1];
      gl->n--;
    } else {
      if (bl->upval && reglevel(fs, gt->nactvar) > outlevel) gt->close = 1;
      gt->nactvar = bl->nactvar;
      igt++;
    }
  }
}

Proto *addprototype (LexState *ls) {
  Proto *clp;
  lua_State *L = ls->L;
  FuncState *fs = ls->fs;
  Proto *f = fs->f;
  if (fs->np >= f->sizep) {
    int oldsize = f->sizep;
    luaM_growvector(L, f->p, fs->np, f->sizep, Proto *, MAXARG_Bx, "functions");
    while (oldsize < f->sizep) f->p[oldsize++] = NULL;
  }
  f->p[fs->np++] = clp = luaF_newproto(L);
  luaC_objbarrier(L, f, clp);
  return clp;
}

void codeclosure (LexState *ls, expdesc *v) {
  FuncState *fs = ls->fs->prev;
  init_exp(v, VRELOC, luaK_codeABx(fs, OP_CLOSURE, 0, fs->np - 1));
  luaK_exp2nextreg(fs, v);
}

void open_func (LexState *ls, FuncState *fs, BlockCnt *bl) {
  lua_State *L = ls->L;
  Proto *f = fs->f;
  fs->prev = ls->fs;
  fs->ls = ls;
  ls->fs = fs;
  fs->pc = 0;
  fs->previousline = f->linedefined;
  fs->iwthabs = 0;
  fs->lasttarget = 0;
  fs->freereg = 0;
  fs->nk = 0;
  fs->nabslineinfo = 0;
  fs->np = 0;
  fs->nups = 0;
  fs->ndebugvars = 0;
  fs->nactvar = 0;
  fs->needclose = 0;
  fs->firstlocal = ls->dyd->actvar.n;
  fs->firstlabel = ls->dyd->label.n;
  fs->bl = NULL;
  f->source = ls->source;
  luaC_objbarrier(L, f, f->source);
  f->maxstacksize = 2;
  fs->kcache = luaH_new(L);
  luaD_anchorobj(L, ls->h, obj2gco(fs->kcache));
  enterblock(fs, bl, 0);
}

void close_func (LexState *ls) {
  lua_State *L = ls->L;
  FuncState *fs = ls->fs;
  Proto *f = fs->f;
  TValue temp;
  luaK_ret(fs, luaY_nvarstack(fs), 0);
  leaveblock(fs);
  lua_assert(fs->bl == NULL);
  luaK_finish(fs);
  luaM_shrinkvector(L, f->code, f->sizecode, fs->pc, Instruction);
  luaM_shrinkvector(L, f->lineinfo, f->sizelineinfo, fs->pc, ls_byte);
  luaM_shrinkvector(L, f->abslineinfo, f->sizeabslineinfo, fs->nabslineinfo, AbsLineInfo);
  luaM_shrinkvector(L, f->k, f->sizek, fs->nk, TValue);
  luaM_shrinkvector(L, f->p, f->sizep, fs->np, Proto *);
  luaM_shrinkvector(L, f->locvars, f->sizelocvars, fs->ndebugvars, LocVar);
  luaM_shrinkvector(L, f->upvalues, f->sizeupvalues, fs->nups, Upvaldesc);
  sethvalue(L, &temp, fs->kcache);
  luaH_set(L, ls->h, &temp, &G(L)->nilvalue);
  ls->fs = fs->prev;
  luaC_checkGC(L);
}

static void mainfunc (LexState *ls, FuncState *fs) {
  BlockCnt bl;
  Upvaldesc *env;
  open_func(ls, fs, &bl);
  luaK_codeABC(fs, OP_VARARGPREP, 0, 0, 0);
  env = allocupvalue(fs);
  env->instack = 1;
  env->idx = 0;
  env->kind = VDKREG;
  env->name = ls->envn;
  luaC_objbarrier(ls->L, fs->f, env->name);
  luaX_next(ls);
  statlist(ls);
  check(ls, TK_EOS);
  close_func(ls);
}

LClosure *luaY_parser (lua_State *L, ZIO *z, Table *anchor, Mbuffer *buff,
                       Dyndata *dyd, const char *name, int firstchar) {
  LexState lexstate;
  FuncState funcstate;
  LClosure *cl;
  lexstate.h = anchor;
  cl = luaF_newLclosure(L, 1);
  luaD_anchorobj(L, anchor, obj2gco(cl));
  funcstate.f = cl->p = luaF_newproto(L);
  luaC_objbarrier(L, cl, cl->p);
  funcstate.f->source = luaS_new(L, name);
  luaC_objbarrier(L, funcstate.f, funcstate.f->source);
  lexstate.buff = buff;
  lexstate.dyd = dyd;
  dyd->actvar.n = dyd->gt.n = dyd->label.n = 0;
  luaX_setinput(L, &lexstate, z, funcstate.f->source, firstchar);
  mainfunc(&lexstate, &funcstate);
  return cl;
}
