/*
** $Id: lauxlib.c $
** Auxiliary functions for building Lua libraries
** See Copyright Notice in lua.h
*/

#define lauxlib_c
#define LUA_LIB

#include "../../include/lprefix.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../include/lua.h"
#include "../../include/lauxlib.h"
#include "../../include/llimits.h"

/*
** {======================================================
** Modular pieces
** =======================================================
*/

#include "laux/laux_traceback.c"
#include "laux/laux_error.c"
#include "laux/laux_meta.c"
#include "laux/laux_check.c"
#include "laux/laux_buffer.c"
#include "laux/laux_ref.c"
#include "laux/laux_load.c"
#include "laux/laux_misc.c"

/* }====================================================== */
