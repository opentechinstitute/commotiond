/**
 *       @file  luawrap.c
 *      @brief  An API wrapper for the Lua programming Language
 *
 *     @author  Luis E. Garcia Ontanon <luis@ontanon.org>
 *
 *     Created  01/07/2014
 *    Revision  $Id:  $
 *    Compiler  gcc/g++
 *   Copyright  Copyright (c) 2013, Luis E. Garcia Ontanon <luis@ontanon.org>
 *
 *   See Copyright notice at bottom of the file
 */


#ifndef __LuaWrap
/* this C include is most likely be dumped into a generated wrapper than compiled
   standalone. The include is unnecessary furthermore the file is likely not
   even to exists at the given location so its fencing.
   */
#include "luawrap.h"
#endif

#ifdef LW_NEED_OPTB
LW_MODE int luaL_optboolean(lua_State* L, int n, int def) {
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

LW_MODE int lwCleanup(void) {
    /* cleanup lua */
    lua_close(L);
    L = NULL;
    return 0;
}

LW_MODE int lwNop(lua_State* L) { return 0; }

LW_MODE lua_State* lwState(void) { return L; }


LW_MODE const char* lwVFmt(const char *f, va_list a) { 
    static char b[ROTBUF_NBUFS][ROTBUF_MAXLEN];
    static int i = 0; 
    vsnprintf(s, ROTBUF_MAXLEN, f, a);
    return s;
}


LW_MODE const char* lwFmt(const char *f, ...)  {
    const char* r;
    va_list a;
    va_start(a, f);
    r = lwVFmt(f,a);
    va_end(a);
    return r;
}

LW_MODE const char* v2s(VS* vs, int v, int def) {
    for (;vs->s;vs++)
        if (vs->v == v)
            return vs->s;
    return def;
}

LW_MODE int s2v(VS* vs, const char* s, int def) {
    for (;vs->s;vs++)
        if ( vs->s == s || strcmp(vs->s,s)==0 )
            return vs->v;
    return def;
}


LW_MODE const gchar* lua_shiftstring(lua_State* L, int i) {
    const gchar* p = luaL_checkstring(L, i);

    if (p) {
        lua_remove(L,i);
        return p;
    } else {
        return NULL;
    }
}

LW_MODE const char* lwCbKeyPtr(const char* cbname, const char* fname, void* ptr) {
    return lwFmt("%s%s%p",cbname,fname,ptr);
}

LW_MODE const char* lwCbKeyS(const char* cbname, const char* fname, void* ptr) {
    return lwFmt("%s%s%s",cbname,fname,(const char*)ptr);
}

LW_MODE const char* lwCbKeyInt(const char* cbname, const char* fname, void* ptr) {
    return lwFmt("%s%s%d",cbname,fname,(int)ptr);
}


/* following is based on the luaL_setfuncs() from Lua 5.2, so we can use it in pre-5.2 */
static void lwSetFuncs(lua_State *L, const luaL_Reg *l, int nup) {
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


LW_MODE int lwClassCreate(luaState* L, const char* name, luaL_Reg* methods, luaL_Reg* meta, lua_CFunction __gc) {
    /* create new class method table and 'register' the class methods into it */ 
    lua_newtable (L);
    lwSetFuncs (L, methods, 0);
    /* add a method-table field named '__typeof' = the class name, this is used by the typeof() Lua func */ 
    pushS(L, name);
    lua_setfield(L, -2, "__typeof");
    /* create a new metatable and register metamethods into it */ 
    luaL_newmetatable (L, name);
    lwSetFuncs (L, meta, 0);
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




/*
Copyright (C) 2014 Luis E. Garcia Ontanon <luis@ontanon.org>

Permission is hereby granted, free of charge, to any person obtaining a copy of this
software and associated documentation files (the "Software"), to deal in the Software
without restriction, including without limitation the rights to use, copy, modify,
merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be included in all copies
or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
*/
