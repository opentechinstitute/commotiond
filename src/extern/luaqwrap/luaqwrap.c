/**
 *       @file  luaqwrap.c
 *      @brief  An API wrapper for the Lua programming Language
 *
 *     @author  Luis E. Garcia Ontanon <luis@ontanon.org>
 *
 *     Created  01/07/2014
 *    Revision  $Id:  $
 *    Compiler  gcc/g++
 *   Copyright  Copyright (c) 2013, Luis E. Garcia Ontanon <luis@ontanon.org>
 *
 *   See LICENSE file for for Copyright notice
 */


#ifndef __LuaQWrap
/* this C include is most likely be dumped into a generated wrapper than compiled
   standalone. The include is unnecessary furthermore the file is likely not
   even to exists at the given location so its fencing.
   */
#include "luaqwrap.h"
#endif

#ifdef LQW_NEED_OPTB
LQW_MODE int luaL_optboolean(lua_State* L, int n, int def) {
    int val = FALSE;

    if ( lua_isboolean(L,n) ) {
        val = lua_toboolean(L,n);
    } else if ( lua_isnil(L,n) || lua_gettop(L) < n ){
        val = def;
    } else {
        luaL_argerror(L,n,"must be a boolean");
    }

    return val;
}
#endif

LQW_MODE lua_State* gL = NULL;

LQW_MODE int lqwCleanup(void) {
    /* cleanup lua */
    lua_close(L);
    L = NULL;
    return 0;
}

LQW_MODE int lqwNop(lua_State* L) { return 0; }

LQW_MODE lua_State* lqwState(void) { return L; }


LQW_MODE const char* lqwVFmt(const char *f, va_list a) { 
    static char b[ROTBUF_NBUFS][ROTBUF_MAXLEN];
    static int i = 0; 
    vsnprintf(s, ROTBUF_MAXLEN, f, a);
    return s;
}


LQW_MODE const char* lqwFmt(const char *f, ...)  {
    const char* r;
    va_list a;
    va_start(a, f);
    r = lqwVFmt(f,a);
    va_end(a);
    return r;
}

LQW_MODE const char* lqwV2S(VS* vs, int v, int def) {
    for (;vs->s;vs++)
        if (vs->v == v)
            return vs->s;
    return def;
}

LQW_MODE int lqwS2V(VS* vs, const char* s, int def) {
    for (;vs->s;vs++)
        if ( vs->s == s || strcmp(vs->s,s)==0 )
            return vs->v;
    return def;
}

LQW_MODE const gchar* lua_shiftstring(lua_State* L, int i) {
    const gchar* p = luaL_checkstring(L, i);

    if (p) {
        lua_remove(L,i);
        return p;
    } else {
        return NULL;
    }
}


/* following is based on the luaL_setfuncs() from Lua 5.2, so we can use it in pre-5.2 */
static void lqwSetFuncs(lua_State *L, const luaL_Reg *l, int nup) {
  luaL_checkstack(L, nup, "too many upvalues");
  for (; l->name != NULL; l++) {  /* fill the table with given functions */
    int i;
    for (i = 0; i < nup; i++)  /* copy upvalues to the top */
      lua_pushvalue(L, -nup);
    lua_pushcclosure(L, l->func, nup);  /* closure with those upvalues */
    lua_setfield(L, -(nup + 2), l->name);
  }
  lua_pop(L, nup);  /* remove upvalues */
}


LQW_MODE int lqwClassCreate(luaState* L, const char* name, luaL_Reg* methods, luaL_Reg* meta, lua_CFunction __gc) {
    /* create new class method table and 'register' the class methods into it */ 
    lua_newtable (L);
    lqwSetFuncs (L, methods, 0);
    /* add a method-table field named '__typeof' = the class name, this is used by the typeof() Lua func */ 
    pushS(L, name);
    lua_setfield(L, -2, "__typeof");
    /* create a new metatable and register metamethods into it */ 
    luaL_newmetatable (L, name);
    lqwSetFuncs (L, meta, 0);
    /* add the '__gc' metamethod with a C-function named Class__gc */ 
    if (__gc) {
        lua_pushcfunction(L, __gc); 
        lua_setfield(L, -2, "__gc");
    }
    /* push a copy of the class methods table, and set it to be the metatable's __index field */
    lua_pushvalue (L, -2);
    lua_setfield (L, -2, "__index");
    /* push a copy of the class methods table, and set it to be the 
       metatable's __metatable field, to hide metatable */ 
    lua_pushvalue (L, -2);
    lua_setfield (L, -2, "__metatable");
    /* pop the metatable */
    lua_pop (L,1);
    return 1;
}
