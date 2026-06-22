/*
** PyLua Parser - Internal Header
** Shares common parser functions across modularized files.
*/

#ifndef lparser_priv_h
#define lparser_priv_h

#include "../../../include/lua.h"
#include "../../../include/lparser.h"
#include "../../../include/llex.h"
#include "../../../include/lcode.h"

/* nodes for block list (list of active blocks) */
typedef struct BlockCnt {
  struct BlockCnt *previous;  /* chain */
  int firstlabel;  /* index of first label in this block */
  int firstgoto;  /* index of first pending goto in this block */
  short nactvar;  /* number of active declarations at block entry */
  lu_byte upval;  /* true if some variable in the block is an upvalue */
  lu_byte isloop;  /* 1 if 'block' is a loop; 2 if it has pending breaks */
  lu_byte insidetbc;  /* true if inside the scope of a to-be-closed var. */
} BlockCnt;

/* Helper Macros (cannot be static if used across files) */
#define check_condition(ls,c,msg)	{ if (!(c)) luaX_syntaxerror(ls, msg); }
#define new_localvarliteral(ls,v) \
    new_localvar(ls,  \
      luaX_newstring(ls, "" v, (sizeof(v)/sizeof(char)) - 1));

/* Prototypes from lparser_core.c */
void luaY_checklimit (FuncState *fs, int v, int l, const char *what);
int testnext (LexState *ls, int c);
void check (LexState *ls, int c);
void checknext (LexState *ls, int c);
void check_match (LexState *ls, int what, int who, int where);
TString *str_checkname (LexState *ls);
void init_exp (expdesc *e, expkind k, int i);
void codestring (expdesc *e, TString *s);
void codename (LexState *ls, expdesc *e);
int new_varkind (LexState *ls, TString *name, lu_byte kind);
int new_localvar (LexState *ls, TString *name);
void adjustlocalvars (LexState *ls, int nvars);
void removevars (FuncState *fs, int tolevel);
void markupval (FuncState *fs, int level);
void marktobeclosed (FuncState *fs);
void singlevaraux (FuncState *fs, TString *n, expdesc *var, int base);
void buildglobal (LexState *ls, TString *varname, expdesc *var);
void buildvar (LexState *ls, TString *varname, expdesc *var);
void singlevar (LexState *ls, expdesc *var);
void adjust_assign (LexState *ls, int nvars, int nexps, expdesc *e);
void enterblock (FuncState *fs, BlockCnt *bl, lu_byte isloop);
void leaveblock (FuncState *fs);
void addprototype_core (LexState *ls, Proto **clp);
void codeclosure (LexState *ls, expdesc *v);
void open_func (LexState *ls, FuncState *fs, BlockCnt *bl);
void close_func (LexState *ls);
Proto *addprototype (LexState *ls); /* Wrapper or direct */
Labeldesc *findlabel (LexState *ls, TString *name, int ilb);
int newlabelentry (LexState *ls, Labellist *l, TString *name, int line, int pc);
int newgotoentry (LexState *ls, TString *name, int line);
void createlabel (LexState *ls, TString *name, int line, int last);
void check_readonly (LexState *ls, expdesc *e);
Vardesc *getlocalvardesc (FuncState *fs, int vidx);
lu_byte reglevel (FuncState *fs, int nvar);
LocVar *localdebuginfo (FuncState *fs, int vidx);
void init_var (FuncState *fs, expdesc *e, int vidx);

/* Prototypes from lparser_expr.c */
void expr (LexState *ls, expdesc *v);
int explist (LexState *ls, expdesc *v);
void constructor (LexState *ls, expdesc *t);
void yindex (LexState *ls, expdesc *v);
void fieldsel (LexState *ls, expdesc *v);
void funcargs (LexState *ls, expdesc *f);

/* Prototypes from lparser_stat.c */
void statement (LexState *ls);
void statlist (LexState *ls);
void block (LexState *ls);
int block_follow (LexState *ls, int withuntil);
void body (LexState *ls, expdesc *e, int ismethod, int line);
void parlist (LexState *ls);
void importstat (LexState *ls);

/* Prototypes from lparser_ctrl.c */
int cond (LexState *ls);
void ifstat (LexState *ls, int line);
void whilestat (LexState *ls, int line);
void forstat (LexState *ls, int line);
void repeatstat (LexState *ls, int line);
void gotostat (LexState *ls, int line);
void breakstat (LexState *ls, int line);
void labelstat (LexState *ls, TString *name, int line);
void test_then_block (LexState *ls, int *escapelist);

#endif
