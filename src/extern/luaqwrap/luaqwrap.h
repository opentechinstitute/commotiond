/**
 *       @file  luawrap.h
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
 *   Based on code written by me <lego@wireshark.org> for Wireshark 
 */

#ifndef __LuaQWrap
#define __LuaQWrap

#include <math.h>
#include <stdio.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#ifndef LQW_MODE
#define LQW_MODE extern
#endif 

/* Sizes */
#ifndef ROTBUF_MAXLEN
# define ROTBUF_MAXLEN 255
#endif 

#ifndef ROTBUF_NBUFS
# define ROTBUF_NBUFS 16
#endif 

/**************************************************************************/
/* Lua shortcuts */
#define toS lua_tostring
#define checkS luaL_checkstring
#define pushS lua_pushstring
#define isS lua_isstring
#define optS luaL_optstring
#define shiftS(L,i) //2do (fn)
#define pushFmtS(ARGS) ( lua_pushstring(L, lqwFmt ARGS ) )

#define toL lua_tolstring
#define checkL luaL_checklstring
#define pushL lua_pushlstring
#define isL lua_isstring
#define optL luaL_optlstring
#define shiftL(L,i) //2do (fn)

#define toN(L,i,T) ((T)lua_tonumber(L,(i)))
#define checkN(L,i,T) ((T)luaL_checknumber(L,(i)))
#define pushN(L,n) (lua_pushNumber(L,(luaNumber)n))
#define isN(L,i) (lua_isnumber(L,(i)))
#define optN(L,i,T,D) ((T)luaL_optnumber(L,(i),(luaNumber)(D)))
#define shiftN(L,i,T) //2do (fn)

#define toB lua_toboolean
#define checkB luaL_checkboolean
#define pushB(L,b) (lua_pushboolean(L,(int)b))
#define isB lua_isboolean
#define optB(L,i,D) (luaL_optboolean(L,(i),(int)(D)))
#define shiftB(L,i) //2do (fn)

#define checkNil luaL_checknil
#define pushNil lua_pushnil
#define isNil lua_isnil
#define shiftNil(L,i) //2do (fn)

#define isF(L,i) ((lua_type(L, i) == LUA_TFUNCTION) || lua_type(L, i) == LUA_TCFUNCTION))

#define isLF(L,i) (lua_type(L, i) == LUA_TFUNCTION)
#define isCF(L,i) (lua_type(L, i) == LUA_TCFUNCTION)

#define pushCF lua_pushcfunction

#define isT(L,i) (lua_type(L, i) == LUA_TTABLE)
#define ckeckT(L,i) do { if(lua_type(L, i) == LUA_TTABLE) ArgError(i,"not a table");
#define pushT lua_newtable

#define getReg(k) lua_getfield(L, LUA_REGISTRYINDEX,(k))
#define setReg(k) lua_setfield(L, LUA_REGISTRYINDEX,(k))
#define getGlob(k) lua_getfield(L, LUA_GLOBALSINDEX,(k))
#define setGlob(k) lua_setfield(L, LUA_GLOBALSINDEX,(k))

#define DLI lua_State* L, int idx
#define DL lua_State* L
#define LI L,idx
#define L1 L,1
#define L2 L,2
#define L3 L,3
#define L4 L,4
#define L5 L,5
#define R0 return 0
#define R1 return 1
#define R2 return 2
#define E int

// Used to allow a semicolon at the end of Def* macros;
// Credit Balint Reczey (rbalint@wireshark.org)
#define _End(C,N) typedef int dummy_##C##_##N

/****************************************************************/
/*
 * ClassDefine(Xxx)
 * defines:
 * toXxx(L,idx) gets a Xxx from an index (Lua Error if fails)
 * checkXxx(L,idx) gets a Xxx from an index after calling check_code (No Lua Error if it fails)
 * optXxx(L,idx,dflt) gets an Xxx from an index, dflt if it is nil or unexistent 
 * pushXxx(L,xxx) pushes an Xxx into the stack
 * isXxx(L,idx) tests whether we have an Xxx at idx
 * shiftXxx(L,idx) removes and returns an Xxx from idx only if it has a type of Xxx, returns NULL otherwise
 * dumpXxx(L,idx) returns a cheap string representation of the Xxx at pos idx 
 * you must define:
 *   void Xxx__check(Xxx*) check the object, throw an error if invalid
 *   void Xxx__push(Xxx*) what to do when a new object is pushed into Lua... 
 *   void Xxx__gcoll(Xxx*) the garbage collector... what to do when the object leaves Lua.
 * LqwDefinePushDummy(Xxx) and LqwDefineChkDummy(Xxx) and LqwDefineGCollDummy(C) might help
 */
#define LqwClassDefine(C) \
LQW_MODE void C##__push(#C*); LQW_MODE void C##__check(#C*); LQW_MODE void C##__gcoll(#C*) \
LQW_MODE C to##C(DLI) { \
    C* v = (C*)lua_touserdata (L, idx); \
    if (!v) luaL_error(L, "bad argument %d (%s expected, got %s)", idx, #C, lua_typename(L, lua_type(L, idx))); \
    return v ? *v : NULL; \
} \
LQW_MODE C check##C(DLI) { \
    C* p; \
    luaL_checktype(DLI,LUA_TUSERDATA); \
    p = (C*)luaL_checkudata(LI, #C); \
    C##__check(p); \
    return p ? *p : NULL; \
} \
LQW_MODE C* push##C(lua_State* L, C v) { \
    C* p; \
    luaL_checkstack(L2,"Unable to grow stack\n"); \
    p = (C*)lua_newuserdata(L,sizeof(C)); *p = v; \
    luaL_getmetatable(L, #C); lua_setmetatable(L, -2); \
    C##__push(p); \
    return p; \
}\
LQW_MODE gboolean is##C(DLI) { \
    void *p; \
    if(!lua_isuserdata(LI)) return FALSE; \
    p = lua_touserdata(LI); \
    lua_getfield(L, LUA_REGISTRYINDEX, #C); \
    if (p == NULL || !lua_getmetatable(DLI) || !lua_rawequal(L, -1, -2)) p=NULL; \
    lua_pop(L2); \
    return p ? TRUE : FALSE; \
} \
LQW_MODE C shift##C(DLI) { \
    C* p; \
    if(!lua_isuserdata(LI)) return NULL; \
    p = (C*)lua_touserdata(LI); \
    lua_getfield(L, LUA_REGISTRYINDEX, #C); \
    if (p == NULL || !lua_getmetatable(L, i) || !lua_rawequal(L, -1, -2)) p=NULL; \
    lua_pop(L2); \
    if (p) { lua_remove(LI); return *p; }\
    else return NULL;\
} \
LQW_MODE int dump#C(lua_State* L) { pushS(L,#C); R1; } \
LQW_MODE C* opt#C(DLI,dflt) { retrun ( (lua_gettop(L)<idx || isNil(LI)) ? dflt : check##C(LI) )); R0;}}\
LQW_MODE int C##__gc (luaState* L) { C#__gcoll(to##C(L,1)); return 0; } \
_End(C,ClassDefine)


#define LqwDefinePushDummy(C) LQW_MODE void C##__push(#C*) {}
#define LqwDefineChkDummy(C) LQW_MODE void C##__check(#C*) {}
#define LqwDefineGCollDummy(C) LQW_MODE void C##__gcoll(#C*) {}


#define LqwClassDeclare(C) \
LQW_MODE C to##C(DLI); \
LQW_MODE C check##C(DLI); \
LQW_MODE C* push##C(lua_State* L, C v) ; \
LQW_MODE gboolean is##C(DLI) ; \
LQW_MODE C shift##C(DLI) ; \
LQW_MODE int dump#C(lua_State* L); \
LQW_MODE C* opt#C(DLI,dflt);
LQW_MODE int C##__gc (luaState* L)
;

/* @brief prototype for callback keying functions
 * @param cbname the name of the callback type
 * @param ptr the datum passed to the callback that will be used to uniquelly indentify each instance
 *
 * the return of a CbKey must be able to uniquely identify the calling instance
 * of a callback based on a datum passed to the callback
 */
typedef const char* (*lqwCbKey)(void* ptr);
#define LqwCbKeyPtr(CB) LQW_MODE const char* CB##_key(void* p) { return lqwFmt(#CB ":%p",p); }
#define LqwCbKeyStr(CB) LQW_MODE const char* CB##_key(void* p) { return lqwFmt(#CB ":%s",(const char*)p); }



/****************************************************************/

/*
 * it creates the class and its tables and leaves it on top of the stack.
 */
LQW_MODE int lqwClassCreate(luaState* L, const char* name, luaL_Reg* methods, luaL_Reg* meta, lua_CFunction __gc);

/* ... to be added to a given table (idx)*/
#define LqwClassRegister(C,idx) { pushS(L,#C); lqwClassCreate(L,#C,C##_methods,C##_meta,C##__gc); lua_settable(L,idx); }

/* ... or to the global namespace */
#define LqwClassRegister(C) { pushS(L,#C); lqwClassCreate(L,#C,C##_methods,C##_meta,C##__gc); lua_setglobal(L,-1); }

/* LuaWrapDeclarations and LuaWrapDefinitions
 */

/* this is quick a malloc-less sprintf()-like replacement, it holds the resulting strings
   in rotating storage, ROTBUF_NBUFS determines how many different strings will be there at any time.
   Results are valid before calling it again ROTBUF_NBUFS times, after that the result is being overwritten.
   */
LQW_MODE const char* lqwFmt(const char *f, ...);
LQW_MODE const char* lqwVFmt(const char *f, va_list);

typedef struct VS {const char* s; int v;} VS;
LQW_MODE const char* lqwV2S(VS* vs, int v, int def);
LQW_MODE int lqwS2V(VS* vs, const char* s, int def);
LQW_MODE int pushE(lua_State* L, int idx, VS* e); // 2do

/* luaL_error() returns void... so it cannot be used in expressions... that sucks!
 * this one returns, so it can be used "inline" with &&, || and ?:
 */
LQW_MODE int lqwError(lua_State* L, const char *f, ...); 
LQW_MODE lua_State* lqwState(void);
LQW_MODE int lqwCleanup(void);

LQW_MODE int lqwNop(lua_State* L); // do nothing

/*
 * RegisterFunction(name)
 * registration code for "bare" functions in global namespace
 */
#define RegisterFunction(name) do { lua_pushcfunction(L, luawrap_## name); lua_setglobal(L, #name); } while(0)


/* function type markers  */
#define Register LQW_MODE int
#define Method LQW_MODE int
#define Constructor LQW_MODE int
#define MetaMethod LQW_MODE int
#define Function LQW_MODE int

/* registration */
#define Methods LQW_MODE const luaL_Reg
#define Meta LQW_MODE const luaL_Reg

/* throws an error */
#define LqwError(error) do { luaL_error(L, error ); return 0; } while(0)

/* throws an error for a bad argument */
#define ArgError(idx,error) do { luaL_argerror(L,idx,error); return 0; } while(0)

/* throws an error for a bad optional argument */
#define OptArgError(idx,error) do { luaL_argerror(L,idx, error); return 0; } while(0)

/* for setting globals */
#define setGlobalB(L,n,v) { pushB(L,v); lua_setglobal(L,n); }
#define setGlobalS(L,n,v) { pushS(L,v); lua_setglobal(L,n); }
#define setGlobalN(L,n,v) { pushN(L,v); lua_setglobal(L,n); }
#define setGlobalO(C,L,n,v) { push##C(L,v); lua_setglobal(L,n); }

#endif
