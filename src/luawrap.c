/* vim: set ts=2 expandtab: */
/**
 *       @file  luawrap.c
 *      @brief  A wrapper for the Lua programming Language
 *
 *     @author  Luis E. Garcia Ontanon <luis@ontanon.org>
 *
 *   @internal
 *     Created  01/07/2014
 *    Revision  $Id:  $
 *    Compiler  gcc/g++
 *     Company  The Open Technology Institute
 *   Copyright  Copyright (c) 2013, Josh King
 *
 * This file is part of Commotion, Copyright (c) 2013, Josh King
 *
 * Commotion is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Commotion is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Commotion.  If not, see <http://www.gnu.org/licenses/>.
 *
 * =====================================================================================
 */

#ifdef HAS_LUA

#include "config.h"
#include "debug.h"
#include "commotion.h"
#include "obj.h"
#include "list.h"
#include "tree.h"
#include "cmd.h"
#include "msg.h"
#include "iface.h"
#include "id.h"
#include "loop.h"
#include "plugin.h"
#include "process.h"
#include "profile.h"
#include "socket.h"
#include "util.h"

#include <math.h>
#include <stdio.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>


/* Sizes */
#define ROTBUF_MAXLEN 255
#define ROTBUF_NBUFS 16

/* Lua shortcuts */
#define toS lua_tostring
#define checkS luaL_checkstring
#define pushS lua_pushstring
#define isS lua_isstring
#define optS luaL_optstring
#define shiftS(L,i) //2do (fn)

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
#define shiftN(L,i) //2do (fn)

#define toB lua_toboolean
#define checkB luaL_checkboolean
#define pushB(L,b) (lua_pushboolean(L,(int)b))
#define isB lua_isboolean
#define optB(L,i,D) (luaL_optboolean(L,(i),(int)(D)))
#define shiftB(L,i) //2do (fn)

#define checkNil luaL_checknil
#define pushNil (lua_pushnil
#define isNil lua_isnil
#define shiftNil(L,i) //2do (fn)

#define isF(L,i) ((lua_type(L, i) == LUA_TFUNCTION) || lua_type(L, i) == LUA_TCFUNCTION))

#define isLF(L,i) (lua_type(L, i) == LUA_TFUNCTION)
#define isCF(L,i) (lua_type(L, i) == LUA_TCFUNCTION)

#define pushCF lua_pushcfunction

#define isT(L,i) (lua_type(L, i) == LUA_TTABLE)
#define ckeckT(L,i) do { if(lua_type(L, i) == LUA_TTABLE) ArgError(i,"not a table");
#define pushT lua_newtable

#define DLI lua_State* L, int idx
#define LI L,idx
#define R1 return 1
#define R0 return 0
#define E int

/****************************************************************/
/*
 * ClassDefine(Xxx,check_code,push_code)
 * defines:
 * toXxx(L,idx) gets a Xxx from an index (Lua Error if fails)
 * checkXxx(L,idx) gets a Xxx from an index after calling check_code (No Lua Error if it fails)
 * optXxx(L,idx,dflt) gets an Xxx from an index, dflt if it is nil or unexistent 
 * pushXxx(L,xxx) pushes an Xxx into the stack
 * isXxx(L,idx) tests whether we have an Xxx at idx
 * shiftXxx(L,idx) removes and returns an Xxx from idx only if it has a type of Xxx, returns NULL otherwise
 * dumpXxx(L,idx) returns a string representation of the Xxx at pos idx 
 * ClassDefine must be used with a trailing ';'
 * (a dummy typedef is used to be syntactically correct)
 */
#define ClassDefine(C,check_code,push_code) \
static C to##C(DLI) { \
    C* v = (C*)lua_touserdata (L, idx); \
    if (!v) luaL_error(L, "bad argument %d (%s expected, got %s)", idx, #C, lua_typename(L, lua_type(L, idx))); \
    return v ? *v : NULL; \
} \
static C check##C(DLI) { \
    C* p; \
    luaL_checktype(DLI,LUA_TUSERDATA); \
    p = (C*)luaL_checkudata(LI, #C); \
    check_code; \
    return p ? *p : NULL; \
} \
static C* push##C(lua_State* L, C v) { \
    C* p; \
    luaL_checkstack(L,2,"Unable to grow stack\n"); \
    p = (C*)lua_newuserdata(L,sizeof(C)); *p = v; \
    luaL_getmetatable(L, #C); lua_setmetatable(L, -2); \
    push_code; \
    return p; \
}\
static gboolean is##C(DLI) { \
    void *p; \
    if(!lua_isuserdata(LI)) return FALSE; \
    p = lua_touserdata(LI); \
    lua_getfield(L, LUA_REGISTRYINDEX, #C); \
    if (p == NULL || !lua_getmetatable(DLI) || !lua_rawequal(L, -1, -2)) p=NULL; \
    lua_pop(L, 2); \
    return p ? TRUE : FALSE; \
} \
static C shift##C(DLI) { \
    C* p; \
    if(!lua_isuserdata(LI)) return NULL; \
    p = (C*)lua_touserdata(LI); \
    lua_getfield(L, LUA_REGISTRYINDEX, #C); \
    if (p == NULL || !lua_getmetatable(L, i) || !lua_rawequal(L, -1, -2)) p=NULL; \
    lua_pop(L, 2); \
    if (p) { lua_remove(LI); return *p; }\
    else return NULL;\
} \
static int dump#C(lua_State* L) { pushS(L,#C); R1; } \
static C* opt#C(DLI,dflt) { retrun ( (lua_gettop(L)<idx || isNil(LI)) ? dflt : check##C(LI) )); R0;}}\
typedef int dummy##C


/** defines garbage collector functions for a Class:
 * markXxx(o) mark an object for garbage collection (to be used when pushing own objects to stack)
 * method xxx.__gc() a garbage collector method for an Xxx
*/
#define DefGC(C)  \
static int mark#C(C* o) { co_obj_setflags(o,(o->flags)|CO_FLAG_LUAGC); } \
static int unmark#C(C* o) { co_obj_setflags(o,(o->flags)&(0xff^CO_FLAG_LUAGC)); } \
static int C##__gc(lua_State* L) { Obj* o=(Obj*)to##C(L,1); if (o&&(o->flags & CO_FLAG_LUAGC)) co_obj_free(o); R0; }


/****************************************************************/
/** Arguments for check_code or push_code
 */
#define FAIL_ON_NULL(s) do { if (! *p) luaL_argerror(L,idx,s); } while(0)
#define CHK_OBJ(Chk,s) do { if ((! *p) || ! Chk(p) ) luaL_argerror(L,idx,s); } while(0)
#define NOP 

/****************************************************************/
/*
 * ClassRegister(Xxx)
 * registration code for a given class
 */
#define ClassRegister(C) { \
    /* check for existing class table in global */ \
    lua_getglobal (L, #C); \
    if (!lua_isnil (L, -1)) { \
        fprintf(stderr, "ERROR: Attempt to register class '%s' which already exists in global Lua table\n", #C); \
        exit(1); \
    } \
    lua_pop (L, 1); \
    /* create new class method table and 'register' the class methods into it */ \
    lua_newtable (L); \
    wslua_setfuncs (L, C ## _methods, 0); \
    /* add a method-table field named '__typeof' = the class name, this is used by the typeof() Lua func */ \
    pushS(L, #C); \
    lua_setfield(L, -2, "__typeof"); \
    /* create a new metatable and register metamethods into it */ \
    luaL_newmetatable (L, #C); \
    wslua_setfuncs (L, C ## _meta, 0); \
    /* add the '__gc' metamethod with a C-function named Class__gc */ \
    /* this will force ALL wslua classes to have a Class__gc function defined, which is good */ \
    lua_pushcfunction(L, C ## __gc); \
    lua_setfield(L, -2, "__gc"); \
    /* push a copy of the class methods table, and set it to be the metatable's __index field */ \
    lua_pushvalue (L, -2); \
    lua_setfield (L, -2, "__index"); \
    /* push a copy of the class methods table, and set it to be the metatable's __metatable field, to hide metatable */ \
    lua_pushvalue (L, -2); \
    lua_setfield (L, -2, "__metatable"); \
    /* pop the metatable */ \
    lua_pop (L, 1); \
    /* set the class methods table as the global class table */ \
    lua_setglobal (L, #C); \
}

/****************************************************************/
/*
 * ClassRegisterMeta(Xxx)
 * registration code for a classes metatable
 */
#define ClassRegisterMeta(C) { \
    /* check for existing metatable in registry */ \
    luaL_getmetatable(L, #C); \
    if (!lua_isnil (L, -1)) { \
        fprintf(stderr, "ERROR: Attempt to register metatable '%s' which already exists in Lua registry\n", #C); \
        exit(1); \
    } \
    lua_pop (L, 1); \
    /* create a new metatable and register metamethods into it */ \
    luaL_newmetatable (L, #C); \
    wslua_setfuncs (L, C ## _meta, 0); \
    /* add a metatable field named '__typeof' = the class name, this is used by the typeof() Lua func */ \
    pushS(L, #C); \
    lua_setfield(L, -2, "__typeof"); \
     /* add the '__gc' metamethod with a C-function named Class__gc */ \
    /* this will force ALL wslua classes to have a Class__gc function defined, which is good */ \
    lua_pushcfunction(L, C ## __gc); \
    lua_setfield(L, -2, "__gc"); \
    /* pop the metatable */ \
    lua_pop(L, 1); \
}


/****************************************************************/
/* more Lua macros */


/*
 * RegisterFunction(name)
 * registration code for "bare" functions in global namespace
 */
#define RegisterFunction(name) do { lua_pushcfunction(L, luawrap_## name); lua_setglobal(L, #name); } while(0)


/* these function type markers might not be useful at all */
#define Register static int
#define Method static int
#define Constructor static int
#define MetaMethod static int
#define Function static int


#define Methods static const luaL_Reg
#define Meta static const luaL_Reg


/* throws an error */
#define Error(error) do { luaL_error(L, error ); return 0; } while(0)

/* throws an error for a bad argument */
#define ArgError(idx,error) do { luaL_argerror(L,idx,error); return 0; } while(0)

/* throws an error for a bad optional argument */
#define OptArgError(idx,error) do { luaL_argerror(L,idx, error); return 0; } while(0)

/* for registering globals */
#define RegGlobalBool(L,n,v) { pushB(L,v); lua_setglobal(L,n); }
#define RegGlobalString(L,n,v) { pushS(L,v); lua_setglobal(L,n); }
#define RegGlobalNumber(L,n,v) { pushN(L,v); lua_setglobal(L,n); }



/****************************************************************/
/*
 * DefMethod_[OSNL]_(_[OSNL])+ (C,M,P,...) for quick definition of wrapping api calls
 * PrefixMethod() ==> Class__Method
 * in name:
 *    _N=number _S=string _O=object _L=buffer _Xo=opt_x 
 * in args:
 *    C=Class P=Prefix O=OutType Sz=BufferSize D=Default T=numberType 
 *    C1,C2=inClass D1,D2=defaults T1,T2=types
 */

#define _M(C,M) Method C##_##M(lua_State* L)

/* int P_M(C*) ==> n = c.f() */
#define DefMethod_N(C,M,P)  { pushN(L,P##M(check##C(L,1))); R1; }
/* O* P_M(C*) ==> o = c.f() */
#define DefMethod_O(C,M,P,O) _M(C,M) { push##O(L,P##M(check##C(L,1))); R1; }
/* char* P_M(C*) ==> s = c.f() */
#define DefMethod_S(C,M,P) _M(C,M) { pushS(L,P##M(check##C(L,1))); R1; }
/* int P_M(C*,char*,size_t) ==> b = c.f() */
#define DefMethod_L(C,M,P,Sz) _M(C,M) { char b[Sz]; pushL(L,b,P##M(check##C(L,1),b,Sz)); R1; }

/*  T P_M(C*, char*) ==> n = c.f(s) */
#define DefMethod_N__S(C,M,P) _M(C,M) { pushN(L,P##M(check##C(L,1),checkS(L,2))); R1; }
/*  T P_M(C*, char*) ==> n = c.f(opt_s) */
#define DefMethod_N__So(C,M,P,D) _M(C,M) { pushN(L,P##M(check##C(L,1),optS(L,2,D))); R1; }
/* T P_M(C*, T) ==> n = c.f(n) */
#define DefMethod_N__N(C,M,P,T) _M(C,M) { pushN(L,P##M(check##C(L,1),checkN(L,2,T))); R1; }
/* T P_M(C*, T) ==> n = c.f(opt_n) */
#define DefMethod_N__No(C,M,P,T,D) _M(C,M) { pushN(L,P##M(check##C(L,1),optN(L,2,T,D))); R1; }
/* T P_M(C*, C2*) ==> n = c.f(o) */
#define DefMethod_N__O(C,N,P,C2) _M(C,M) { pushN(L,P##M(check##C(L,1), check##C2(L,2))); R1; }
/* T P_M(C*, C2*) ==> n = c.f(o) */
#define DefMethod_N__Oo(C,N,P,C2,D2) _M(C,M) { pushN(L,P##M(check##C(L,1), opt##C2(L,2,D2))); R1; }

/* O* P_M(C*, char*) ==> o = c.f(s) */
#define DefMethod_O__S(C,M,P,O) _M(C,M) { push##O(L,P##M(check##C(L,1),checkS(L,2))); R1; }
/* O* P_M(C*, char*) ==> o = c.f(opt_s) */
#define DefMethod_O__So(C,M,P,O) _M(C,M) { push##O(L,P##M(check##C(L,1),checkS(L,2))); R1; }
/* O* P_M(C*, T) ==> o = c.f(n) */
#define DefMethod_O__N(C,M,P,O,T) _M(C,M) { push##O(L,P##M(check##C(L,1),checkN(L,2,T))); R1; }
/* O* P_M(C*, T) ==> o = c.f(opt_n) */
#define DefMethod_O__No(C,M,P,O,T,D) _M(C,M) { push##O(L,P##M(check##C(L,1),optN(L,2,T,D))); R1; }
/* O* P_M(C*, C2*) ==>  o = c.f(o) */
#define DefMethod_N__O(C,N,P,C2) _M(C,M) { push##O(L,P##M(check##C(L,1), check##C2(L,2))); R1; }
/* O* P_M(C*, C2*) ==>  o = c.f(opt_o) */
#define DefMethod_O__Oo(C,N,P,O,C2,D) _M(C,M) { push##O(L,P##M(check##C(L,1), opt##C2(L,2,D))); R1; }


/* n = c.f(s1,s2) */
#define DefMethod_N__S_S(C,M,P) _M(C,M) { pushN(L,P##M(check##C(L,1),checkS(L,2),checkS(L,3))); R1; }
/* n = c.f(s1,opt_s2) */
#define DefMethod_N__S_So(C,M,P,D2) _M(C,M) { pushN(L,P##_##M(check##C(L,1),checkS(L,2),optS(L,3,D2))); R1; }
/* n = c.f(opt_s1,opt_s2)  */
#define DefMethod_N__S_So(C,M,P,D1,D2) _M(C,M) { pushN(L,P##_##M(check##C(L,1),optS(L,2,D1),optS(L,3,D2))); R1; }


/* n = c.f(s1,s2,s3) */
#define DefMethod_N__S_S_S(C,M,P) _M(C,M) { pushN(L,P##_##M(check##C(L,1),checkS(L,2),checkS(L,3),checkS(L,4))); R1; }



/* T P_N(C*,C1*,C2*) ==> n = c.f(opt_o1,opt_o2) */
#define DefMethod_N__Oo_Oo(C,N,P,O1,O2) _M(C,M) { pushN(L,P##M(check##C(L,1),opt##C3(L,3), opt##C2(L,2))); R1; }

/* T P_N(C*,C1*,C2*) ==> n = c.f(opt_o1,opt_o2) */
#define DefMethod_N__O_Oo(C,M,P,O1,O2) _M(C,M) { pushN(L,P##M(check##C(L,1), check##C2(L,2), opt##C3(L,3))); R1; }

/*  O* P_N(C*,C1*,C2*) ==>  o = c.f(opt_o1,opt_o2) */
#define DefMethod_O__Oo_Oo(C,M,P,O,C1,D1,C2,D2) _M(C,M){ push##O(L,P##M(check##C(L,1),opt##C1(L,2,D1),opt##C2(L,3,D2))); R1; }

/*  O* P_N(C*,C1*,C2*) ==>  o = c.f(o1,opt_o2) */
#define DefMethod_O__O_Oo(C,N,P,O,C1,D1,C2,D2) _M(C,M) { push##O(L,P##N(check##C(L,1),check##D1(L,2,D1),opt##C2(L,3,D2))); R1; }


/* O* P_N(C*,C2*,T) ==> o = c.f(o,n) */
#define DefMethod_O__O_N(C,M,P,O,C2,T) _M(C,M) { push##O(L,P##M(check##C(L,1),check#C2(L,2),checkN(L,3,T))); R1; }

#define DefMethod_N_L(C,M,P) _M(C,M) { size_t len; pushN(P##M(check##C(L,1), checkL(L,2,&len), len); R1; }

#define DefMethod_L(C,M,P,Sz) _M(C,M) { char b[Sz]; pushL(L,b,P##_##M(check##C(L,1),b,Sz)); R1; }

#define DefMethod_L__O(C,M,P,Sz,O) _M(C,M) { char b[Sz]; pushL(L,b,P##_##M(check##C(L,1),check##O(L,2),b,Sz)); R1; }

#define DefMethod_L__N_N(C,M,P,Sz,O,T1,T2) _M(C,M) { char b[Sz]; pushL(L,b,P##_##M(check##C(L,1), checkN(L,2,T1), checkN(L,3,T2), b, Sz))); R1 }

#define DefMethod_N__N_N_L(C,M,P,) _M(C,M) { size_t len=0; pushN(L,P##_##M(check##C(L,1), checkN(L,2,T1), checkL(L,3,T2), checkL(L, 4,&len), len)); R1; }

/* quick methods for struct accessors */

/* Accesses string from struct Elem   */
#define DefAccessorRW_S(C,E) Method C##_##E(lua_State* L) { C* o=check##C(L,1); \
    if (isS(L,2)) { /*o->E = toS(L,2); */ R0; } else { pushS(L,o->E); R1;} }
#define DefAccessorRO_S(C,E) Method C##_##E(lua_State* L) { pushS(L,check##C(L,1)->E); R1; }


/* Accesses number from struct Elem  */
#define DefAccessorRW_N(C,E,T) Method C##_##E(lua_State* L) { C* o = check##C(L,1); \
    if ( isN(L,2) ) { o->E = toN(L,2,T); R1; } else { pushN(L,o->E); R1;} }
#define DefAccessorRO_N(C,E,T) Method C##_##E(lua_State* L) { pushN(L,check##C(L,1)->E); R1; }

/* Accesses enum from struct Elem  */
#define DefAccessorRW_E(C,E,Vs,D) Method C##_##E(lua_State* L) { C* o = check##C(L,1); \
    if ( isS(L,2) ) { o->E = S2V(Vs,toS(L,2),D); R1; } else { pushE(L,o->E,Vs); R1;} }
#define DefAccessorRO_E(C,E,Vs) Method C##_##E(lua_State* L) { pushE(L,check##C(L,1)->E,Vs); R1; }

/* Accesses bool from struct Elem  */
#define DefAccessorRW_B(C,E) Method C##_##E(lua_State* L) { C* o = check##C(L,1); \
    if (isB(L,2)) { o->E = toB(L,2); lua_settop(L,2); R1; } else { pushB(L,o->E); R1;} }
#define DefAccessorRO_B(C,E) Method C##_##E(lua_State* L) { pushB(L,(int)check##C(L,1)->E); R1; }

/* Accesses O from struct Elem  */
#define DefAccessorRW_O(C,E,O) Method C##_##E(lua_State* L) { C* o = check##C(L,1); \
    if (is##O(L,2)) { o->E = to##O(L,2); R1; } else { push##O(L,o->E); R1;} }
#define DefAccessorRO_O(C,E,O) Method C##_##E(lua_State* L) { push##O(L,check##C(L,1)->E); R1; }



/****************************************************************/
/* Macros for callback wrappers (used by Process and Socket) */
/* C Class 
 * N Name
 * Id Unique Identifier element in struct
 * T type of number
 * O Type of object
 */
#define SelfCbErr() do { return 0; } while(0) //2do

#define SelfCbKey(C,n1,n2) (cbkey("CB:" #C,n1,n2)))

#define DefSelfCb_N(C,N,Id,T) static T C##_##N##_cb(co_obj_t *self) { \
    lua_getfield(L, LUA_REGISTRYINDEX,SelfCbKey(#C,#N,((C*)self)->Id)); \
    push##C(L,self); \
    if(lua_pcall(L,1,1)){ SelfCbErr(); } else return (T)toN(L,-1); }

#define DefSelfCb_N__O(C,N,Id,T,O) static T C##_##N##_cb(co_obj_t *self, co_obj_t *o) { \
    lua_getfield(L, LUA_REGISTRYINDEX,SelfCbKey(#C,#N,((C*)self)->Id)); \
    push##C(L,self); push#O(L,o); \
    if(lua_pcall(L,2,1)){ SelfCbErr(); } else return toN(L,-1,T); }

#define DefSelfCb_N__S(C,N,Id,T) static T C##_##N##_cb(co_obj_t *self, const char *s) { \
    lua_getfield(L, LUA_REGISTRYINDEX,SelfCbKey(#C,#N,((C*)self)->Id)); \
    push##C(L,self); pushS(L,s); \
    if(lua_pcall(L,2,1)){ SelfCbErr(); } else return toN(L,-1,T); }

#define DefSelfCb_N__L(C,N,Id,T) static T C##_##N##_cb(co_obj_t *self, char *s, size_t len) { \
    lua_getfield(L, LUA_REGISTRYINDEX,SelfCbKey(#C,#N,((C*)self)->Id)); \
    push##C(L,self); pushL(L,s,len); \
    if(lua_pcall(L,2,1)){ SelfCbErr(); } else return toN(L,-1,T); }

#define DefSelfCb_N__O_L(C,N,Id,T,O) static T C##_##N##_cb(co_obj_t *self, co_obj_t *o, char *s, size_t len) { \
    lua_getfield(L, LUA_REGISTRYINDEX,SelfCbKey("Socket",#N,((C*)self)->Id)); \
    push##C(L,self); push##O(L,o); pushL(L,s,len); \
    if(lua_pcall(L,3,1)){ SelfCbErr(); } else return toN(L,-1,T); }

#define DefSelfCb_Ou(C,N,Id,T,O) static T C##_##N##_cb(co_obj_t *self, co_obj_t *o, co_obj_t *unused) { \
    lua_getfield(L, LUA_REGISTRYINDEX,SelfCbKey("Socket",#N,((C*)self)->Id)); \
    push##C(L,self); push##O(L,o);\
    if(lua_pcall(L,2,1)){ SelfCbErr(); } else return toN(L,-1,T); }

#define DefSelfCb_N__N_N_L(C,N,Id,T,T1,T2,PT,LT) static T C##_##N##_cb(C*o, T1 i1, T2 i2, PT *s, LT len) { \
    lua_getfield(L, LUA_REGISTRYINDEX,SelfCbKey("Socket",#N,((C*)self)->Id)); \
    push##C(L,self); pushN(L,i1,T1); pushN(L,i2,T2); pushL(L,s,len);\
    if(lua_pcall(L,4,1)) { SelfCbErr(); } else return toN(L,-1,T); }

/****************************************************************/
/*
 * typedefs for Lua classes
 */

typedef co_obj_t Obj;
typedef co_iface_t Iface;
typedef ListIteration Ifaces;
typedef co_cmd_t Cmd;

typedef union List {
    co_list16_t list16;
    co_list32_t list32;
} List;

typedef co_timer_t Timer;
typedef co_process_t Process;


//2do
typedef union Tree {
    co_tree16_t tree16;
    co_tree32_t tree32;
} Tree;

typedef co_plugin_t Plugin;
typedef co_cbptr_t CallBack;
typedef co_profile_t Profile;
typedef co_socket_t Socket;
typedef unix_socket_t USock;
typedef co_fd_t FileDes;

typedef struct Sys {
    co_exttype_t _header;
} Sys;


/****************************************************************/
/*
 * Class Definitions
 */

ClassDefine(Iface,NOP,NOP);
ClassDefine(Ifaces,NOP,NOP);
ClassDefine(Cmd,NOP,NOP);
ClassDefine(Iterator,NOP,NOP);
ClassDefine(List,NOP,NOP);
ClassDefine(ListIteration,NOP,NOP);
ClassDefine(Timer,NOP,NOP);

//2do
ClassDeclare(Plugin);
ClassDeclare(Process);
ClassDeclare(CallBack);
ClassDeclare(Profile);
ClassDeclare(Socket);
ClassDeclare(USock);
ClassDeclare(FileDes);

ClassDeclare(Sys);

/****************************************************************/
/* internal types*/
typedef struct VS {
    int v,
    const char* s
} VS;

/****************************************************************/
/*
 *  file globals;
 */
static lua_State* gL = NULL;
static co_obj_t* nil_obj = NULL;
static const char* err = NULL;

/****************************************************************/
/* Util funcs */

/* keep in mind that the result will not change for ROTBUF_NBUFS calls. */
const char* tmpFmt(const char *format, ...) {
    static char buf[ROTBUF_NBUFS][ROTBUF_MAXLEN];
    static int bi = 0;
    const char* s = buf[++bi, bi %= ROTBUF_NKEYS];
    va_list args;

    va_start(args, format);
    result = vsnprintf(s, ROTBUF_MAXLEN, format, args);
    va_end(args);

    return s;
}

static const char* v2s(VS* vs, int v, int def) {
    for (;vs->s;vs++) if (vs->v == v) return vs->s;
    return def;
}

static int s2v(VS* vs, const char* s, int def) {
    for (;vs->s;vs++) if ( vs->s == s || strcmp(vs->s,s)==0 ) return vs->v;
    return def;
}

static int pushE(lua_State* L, int idx, VS* e) {};


#define pushFmtS(ARGS) ( lua_pushstring(L, tmpFmt ARGS ) )
#define cbkey(a, b, c) tmpFmt("%s_%s_%s", a, b, c);
#define cat(a,b) tmpFmt("%s%s", a, b);



/****************************************************************/
/* Class Obj: the undeclared class for generic co_obj_t* pointers
   a catch all handler of all userdata...
   every pointer that is not a co_obj_t* is to be dealt as lightuserdata
  */

#define isObj(L,idx) (lua_type(L,idx)==LUA_TUSERDATA)
#define checkObj(L, idx) ( isObj(L,idx)?lua_touserdata(L,idx):luaL_checkudata(L,idx,"MustBeAnObject")) )
#define optObj(L,idx,dflt) ( (lua_gettop(L)<idx || isNil(L,idx)) ? dflt : checkObj(L,idx) )

static co_obj_t* toObj(lua_State* L, int idx) {
    co_obj_t* o = NULL;

    switch(lua_type(L,idx)) {
        case LUA_TNUMBER:
            return co_float64_create(lua_tonumber(L,idx),CO_FLAG_LUAGC);
        case LUA_TBOOLEAN:
            return co_bool_create(lua_toboolean(L,idx),CO_FLAG_LUAGC);
        case LUA_TSTRING: do {
                size_t len=0;
                const char * s = lua_tolstring(L, idx, &len);
                return co_str32_create(s,len,CO_FLAG_LUAGC);
            } while(0);
            break;
        case LUA_TUSERDATA:
            return lua_touserdata(L,idx);
        case LUA_TTABLE:
        case LUA_TFUNCTION:
        case LUA_TTHREAD:
        case LUA_TLIGHTUSERDATA:    
        default:
            Warn("Unimplemented");
        case LUA_TNIL:
            o = co_nil_create(CO_FLAG_LUAGC);
            break;
    }           
    return o;
}

static co_obj_t* shiftObj(lua_State* L, int idx) {
    co_obj_t* p;
    
    if(!lua_isuserdata(L,idx))
        return NULL;
    
    p = toObj(L,idx);
    lua_remove(L,idx);
    
    return *p;
}

static int gcObj(lua_State* L,int idx) {
    co_obj_t* o = NULL;
    
    if(lua_type(L,idx) == LUA_TUSERDATA) 
        o = lua_touserdata(L,idx);
    else {
        Error()
        return 0;
    }
    
    if ( o && (o->flags & CO_FLAG_LUAGC) ) switch (o->type) {
        case _nil: 
        case _true: 
        case _false: 
        case _float32: 
        case _float64:
        case _uint8:
        case _uint16:
        case _uint32:
        case _int8:
        case _int16:
        case _int32:
        case _bin8:
        case _bin16:
        case _bin32:
        case _str8:
        case _str16:
        case _str32:
        case _list16:
        case _list32:
        case _tree16:
        case _tree32:
        case _int64:
        case _uint64:        
        case _fixext1:
        case fixext2:
        case _fixext4:
        case _fixext8:
        case _fixext16:
        
        case _ext8:
            switch( ((co_ext8_t)o)->_exttype) {
                case _cmd: 
                case _iface: 
                case _plug:
                case _profile:
                case _cbptr:
                case _fd: 
                case _sock:
                case _co_timer:
                case _process:
                case _ptr:
                    co_obj_free(o);
                    return 0;
                default: goto error;
            }
        case _ext16:
        case _ext32:
            co_obj_free(o);
            return 0;

        default: goto error;
    }
    
    error:
    return 0;
}

static int pushObj(lua_State* L, co_obj_t* o) {
#define PUSH_N_DATA(T, L) lua_pushnumber(L,(luaNumber)(((T##L##_t)o)->data))
#define PUSH_C_DATA(T, L) lua_pushlstring(L,(((T##L##_t)o)->len),(((T##L##_t)o)->data))
    
    int len = 0;
    char* data;
    switch (o->type) {
        
        case _nil: lua_pushnil(L); R1;
        case _true: lua_pushbool(L,1); R1;
        case _false: lua_pushbool(L,0); R1;

        case _float32: PUSH_N_DATA(float,32); R1;
        case _float64: PUSH_N_DATA(float,64); R1;
        
        case _uint8: PUSH_N_DATA(uint,8); R1;
        case _uint16: PUSH_N_DATA(uint,16); R1;
        case _uint32: PUSH_N_DATA(uint,32); R1;
        
        case _int8: PUSH_N_DATA(int,8); R1;
        case _int16: PUSH_N_DATA(int,16); R1;
        case _int32: PUSH_N_DATA(int,32); R1;

        case _bin8: PUSH_C_DATA(bin, 8); R1;
        case _bin16: PUSH_C_DATA(bin, 16); R1;
        case _bin32: PUSH_C_DATA(bin, 32); R1;
        
        case _str8: PUSH_C_DATA(str, 8); R1;
        case _str16: PUSH_C_DATA(str, 16); R1;
        case _str32: PUSH_C_DATA(str, 32); R1;
        
        case _list16:
        case _list32:
            pushList(L,o);
            R1;
        
        case _tree16:
        case _tree32:
            pushTree(L,o);
            R1;
        
        case _ext8:
        case _ext16:
        case _ext32:
            switch( ((co_ext8_t)o)->_exttype) {
                case _cmd: pushCmd(L,o); R1;
                case _iface: pushIface(L,o); R1;                    
                case _plug:  pushPlug(L,o); R1;
                case _profile: pushProfile(L,o); R1;
                case _cbptr:  pushCallBack(L,o); R1;
                case _fd:  pushFd(L,o); R1;
                case _sock: pushSock(L,o); R1;
                case _co_timer: pushTimer(L,o); R1;
                case _process: pushProcess(L,o); R1;
                case _ptr: return 0;
                default: goto error;
            }
        
        case _int64:
        case _uint64:        
        case _fixext1:
        case fixext2:
        case _fixext4:
        case _fixext8:
        case _fixext16:
        default:
        goto error;
    }
    error:
        lua_pushnil(L);
        R1;
}


/**
 iterator invoking lua code
    the function to be called must be on top of the stack
*/
static co_obj_t* luawrap_iter(co_obj_t *data, co_obj_t *current, void *context) {
    lua_State* L = (lua_State*)context;
    pushObj(L,current); 
    if(lua_pcall(L,1,1)){ err = toS(L,-1); return NULL; }
    lua_pop(L,1);
    return current;
}

/** callback **/

/* for registered callback */

/* function on top of the stack */
static int luawrap_quick_cb(co_obj_t *self, co_obj_t **output, co_obj_t *params) {
    co_str8_t s = ((co_str8_t)self);
    pushObj(gL,params);
    if(lua_pcall(L,1,1)){err=toS(gL,-1); return 0;}
    *output = shiftObj(gL,1);
    rerurn *output?1:0;
}




/****************************************************************/
/*! @class Cmd
 *  @brief a mechanism for registering and controlling display of commands
 */
// ASSUMING that the co_cmd_t* persists over time
// XXX i.e. if the implementation moves the data around this is broken

static int cmd_cb(co_obj_t *self, co_obj_t **output, co_obj_t *params) {
    lua_State* L = (lua_State*)context;
    co_str8_t s = ((co_str8_t)self);
    const char* name = (char*)s->data;
    const char* nlen = (char*)s->len;
    
    pushCmd(co_cmd_get(name, nlen));
    pushObj(gL,params);
    lua_getfield(gL, LUA_REGISTRYINDEX, cbkey("Cmd","-",name)); // function_name

    if(lua_pcall(L,2,1)){err=toS(gL,-1); return 0;}
    *output = shiftObj(gL,1);
    rerurn *output?1:0;
}

/** @constructor new
 * @brief creates and registers a new command
 * @param name: of the command
 * @param usage: of the command
 * @param desc: of the command
 * @param action: function(self,params) to be called when executed
 */
Constructor Cmd_register(lua_State* L) {
    size_t nlen, ulen, dlen;    
    const char *name = luaL_checklstring(L,1,&nlen);
    const char *usage = luaL_checklstring(L,2,&ulen);
    const char *desc = luaL_checklstring(L,3,&dlen);
    int r;
    isF(L,4);

    lua_setfield(L,LUA_REGISTRYINDEX, cbkey("Cmd","-",name));
    
    if (( r = co_cmd_register(name, nlen, usage, ulen, desc, dlen, cmd_cb) != 0 )) {
            pushCmd(L,co_cmd_get(name, nlen));
            R1;
    }
    
    Error(tmpFmt("Cmd.register: fail_code=%d",r));
    return 0;
}

/** @constructor get
 * @brief loads a registered command
 * @param name: of the command
 */
Constructor Cmd_get(lua_State* L) {
    size_t nlen = 0;
    const char *name = luaL_checklstring(L,1,&nlen);
    pushCmd(L,co_cmd_get(name, nlen));
    R1;
}

/** @method exec
 * @brief executes a command
 * @param params: the parameters to the command
 * returns the output of the excution
 */
Method Cmd_exec(lua_State* L) {
    Cmd* c = checkCmd(L,1);
    Obj* param = checkObj(L, 2);
    Obj* key = co_str8_create(c->name, strlen(c->name), 0);
    
    if (co_cmd_exec(key, &output, param) != 0) {
        co_obj_free(key);
        pushObj(L,output);
        R1;
    }

    co_obj_free(key);
    return 0;
}

/** @method usage
 * @brief returns the usage of the command
 */
DefAccessorR_S(Cmd,usage,co_cmd);
           
/** @method desc
 * @brief returns the description of the command
 */
DefAccessorR_S(Cmd,desc,co_cmd);

/** @method hook
 * @brief adds a callback to a command
 * @param action: function to be called
 */
Method Cmd_hook(lua_State* L) {
    Cmd* c = checkCmd(L,1);
    Obj* key = co_str8_create(c->name, strlen(c->name), 0);
    
    luaL_checktype(L, 2, LUA_TFUNCTION);
    lua_setfield (L, LUA_REGISTRYINDEX, cbkey("Cmd","",c->name));
    
    lua_pushboolean(L,co_cmd_hook(key, luawrap_cb));
    R1;
}

/** @method __tostring
 * @brief returns the name of the command
 */
MetaMethod Cmd__tostring(lua_State* L) {
    Cmd* c = checkCmd(L,1);
    pushFmtS(("Cmd:%s",c->name));
    R1;
}

/** @fn process
 * @brief processes all registered commands with given iterator
 * @param iter iterator function reference
 */
Function Cmd_process(lua_State* L) {
    luaL_checkfunction(L,1)
    luawrap_iter_check(L);
    co_cmd_process(luawrap_iter, L);
    R0;
}

//2do: remove? GC?
           
static const luaL_Reg Cmd_methods[] = {
    {"register", Cmd_new},
    {"get", Cmd_get},
    {"exec", Cmd_exec},
    {"usage", Cmd_usage},
    {"desc", Cmd_desc},
    {"hook", Cmd_hook},
    { NULL, NULL }
};

static const luaL_Reg Cmd_meta[] = {
    {"__tostring", Cmd__tostring},
    {"__call", Cmd_exec},
    { NULL, NULL }
};


/****************************************************************/
/*! @class List
 *  @brief Commotion double linked-list implementation
 */

/** @method length
 * @brief returns the number of elements in the list
 */
DefMethod_N(List,lenght,co_list);

/** @method get_first
 * @brief returns the fisrt element of the list
 */
DefMethod_O(List,get_first,co_list,Obj);

/** @method get_first
 * @brief returns the last element of the list
 */
DefMethod_O(List,get_last,co_list,Obj);

/** @method contains
 * @brief returns true if the list contains the given elem
 * @param elem: the element to be looked for
 */
Defmethod_B__O(List,contains,co_list,Obj);

/** @method delete
* @brief returns true if the given elem was removed from the list
* @param elem: the element to be deleted
*/
DefMethod_O__O(List,delete,co_list,Obj);

/** @method insert_before
 * @brief inserts an element before the given element
 * @param elem: the element before which to insert
 * @param new_elem: the element to be inserted
 */
DefMethod_N__O_O(List,insert_before,co_list,Obj,Obj);

/** @method insert_after
 * @brief inserts an element after the given element
 * @param elem: the element after which to insert
 * @param new_elem: the element to be inserted
 */
DefMethod_N__O_O(List,insert_after,co_list,Obj,Obj);

/** @method prepend
 * @brief inserts an element at the start of the list
 * @param new_elem: the element to be inserted
 */
DefMethod_N__O(List,prepend,co_list,Obj);

/** @method append
 * @brief inserts an element at the end of the list
 * @param new_elem: the element to be inserted
 */
DefMethod_N__O(List,append,co_list,Obj);

/** @method element
 * @brief returns the indexth element of the list
 * @param index: the element
 */
DefMethod_O__O_N(List,element,co_list,Obj,int);


/** @method __tostring
 * @brief returns a string representation of the list
 */
MetaMethod List__tostring(lua_State* L) {
    co_obj_t* list = checkList(L,1);
   
    size_t olen = 255;
    char buff[olen];
    
    olen = co_list_raw(buff, olen, list)
    lua_pushlstring(L,buff,olen);
    R1;
}

/** @method foreach
 * @brief calls the given function for each element of the list
 * @param action: function to be called for each object
 * returns a boolean true if all elements were processed
 * action takes the current object and returns a boolean (true to continue)
 */
int List_foreach(lua_State* L) {
    int cont = 0;
    co_obj_t* list = checkList(L,1);
    co_obj_t* cur;
    void* cookie = NULL;
    luaL_checkfunction(L,2);
    
    while( cur = co_list_foreach(list, &cookie) ) {
        pushObj(cur);
        lua_call(L,1,1);
        cont = lua_toboolean(L,-1);
        lua_pop (L, 1);
        if (!cont) break; 
    }
    
    lua_pushboolean(L,cont);
    R1;
}

/** @method __call
 * @brief returns an iterator function
 * function will return next element of the list at each call, nil when done
 */
/** @method iter
 * @brief returns an iterator function
 * function will return next element of the list at each call, nil when done
 */
MetaMethod List__call (lua_State *L) {
  co_obj_t* list = checkList(L,1);
  lua_pushlightudata(L, NULL);
  lua_pushcclosure(L, _List_iter, 2);
  R0;
}

/** @constructor List.import(str)
 * @brief constructs a list given its string representation
 */
Constructor List_import(lua_State* L) {
    size_t ilen=0;
    co_obj_t* list = NULL;
    const char* buff = luaL_checklstring (L, 2, &ilen);

    size_t olen = co_list_import(&list, buff, list);
        
    if (list) pushList(L,markList(list)); else lua_pushnil(L);
    R1;
}

Function _List_iter(lua_State *L) {
  co_obj_t* list = toList(L,1);
  void* next = lua_touserdata(L,2);
  co_obj_t* item = co_list_foreach(list,&next);
  lua_pop(L,1);
  lua_pushlightuserdata(L, next);

  if (item) {
    pushObj(L,item);
    R1;
  } else {
    R0;
  }
}

static const luaL_Reg List_methods[] = {
    {"lenght", List_length},
    {"get_first", List_get_first},
    {"get_last", List_get_last},
    {"for_each", List_ForEach},
    {"contains", List_contains},
    {"delete", List_delete},
    {"insert_before", List_insert_before},
    {"insert_after", List_insert_after},
    {"prepend", List_prepend},
    {"append", List_append},
    {"element", List_element},
    {"import", List_import},
    {"iter", List__call},
    { NULL, NULL }
};

static const luaL_Reg List_meta[] = {
    {"__tostring", List__tostring},
    {"__call", List__call},
    {"__index", List_element},
    {"__len", List_length},
    {"__gc", gcList},
    { NULL, NULL }
};


/****************************************************************/
/*! @class Iface 
 * @brief interface handling for the Commotion daemon
 */


/** @method Iface.wpa_connect()
  * @brief connects the commotion interface to wpa supplicant
  */
DefMethod_N(Iface,wpa_connect,co_iface);

/** @method Iface.wpa_disconnect()
  */
DefMethod_N(Iface,wpa_disconnect,co_iface);

/** @method Iface.set_ip(addr,mask)
  */
DefMethod_N__S_S(Iface,set_ip,co_iface);

/** @method Iface.unset_ip()
  */
DefMethod_N(Iface,unset_ip,co_iface);

/** @method Iface.set_ssid(ssid)
  */
DefMethod_N__S(Iface,set_ssid,co_iface);

/** @method Iface.set_bssid(bssid)
  */
DefMethod_N__S(Iface,set_bssid,co_iface);

/** @method Iface.set_frequency(freq)
  */
DefMethod_N__N(Iface,set_frequency,co_iface,int);

/** @method Iface.set_encription(enc)
  */
DefMethod_N__N(Iface,set_encription,co_iface,int);

/** @method Iface.set_key(key)
  */
DefMethod_N__S(Iface,set_key,co_iface);

/** @method Iface.set_mode(mode)
  */
DefMethod_N__S(Iface,set_mode,co_iface);

/** @method Iface.set_apscan(apscan_mode)
  */
DefMethod_N__N(Iface,set_apscan,co_iface);

/** @method Iface.wireless_enable()
  */
DefMethod_N(Iface,wireless_enable,co_iface);

/** @method Iface.wireless_disable()
  */
DefMethod_N(Iface,wireless_disable,co_iface);

/** @method Iface.set_dns(a,b,c)
  */
DefMethod_N__S_S_S(Iface,set_dns,co_iface);

/** @method Iface.get_mac()
  */
//DefMethod_GET_BUF(Iface,get_mac,co_iface,6);
//2do: fix macro

/** @method Iface.status()
  */

DefAccessorRO_B(Iface,status);

/** @method Iface.wpa_id()
  */
DefAccessorRO_N(Iface,wpa_id)

/** @method Iface.wireless()
  */
DefAccessorRO_B(Iface,wireless)
    
// 2do struct ifreq ifr;
// 2do  struct wpa_ctrl *ctrl;


/** @fn Iface.iter()
  * @brief returns an iterator for all interfaces
  */
Function Iface_iter (lua_State *L) {
  pushList(L,co_ifaces_list());
  lua_pushlightuserdata(L, NULL);
  lua_pushcclosure(L, _List_iter, 2);
  R1;
}

/** @fn Iface.foreach(action)
  * @brief invokes a function for each interface
  * @param action: function taking each object returning true to continue
  * returns true if it has gone through all interfaces
  */
Function Iface_foreach (lua_State *L) {
    int cont = 0;
    co_obj_t* list = co_ifaces_list();
    co_obj_t* cur;
    void* cookie = NULL;
    
    luaL_checkfunction(L,lua_gettop(L));
    
    while( cur = co_list_foreach(list, &cookie) ) {
        pushIface(cur);
        lua_call(L,1,1);
        cont = lua_toboolean(L,-1);
        lua_pop(L, 1);
        if (!cont) break; 
    }
    
    lua_pushboolean(L,cont);
    R1;
}

/** @fn Iface.remove(iface_name)
  * @brief remove an interface
  * @param iface_name: the name of the interface to be removed
  */
Function luawrap_iface_remove(lua_State* L) {
  const char *iface_name = checkS(L,1);
  lua_pushnumber(L,co_iface_remove(iface_name));
  R1;
}

/** @fn Iface.profile(iface_name)
  * @brief remove an interface
  * @param iface_name: the name of the interface to be removed
  */
Function luawrap_iface_profile(lua_State* L) {
  const char *iface_name = checkS(L,1);
  pushS(L,co_iface_profile(iface_name));
  R1;
}

/** @constructor Iface.get(name)
  * @brief get an interface by name
  * @param name: iface_name the name of the interface
  */
Constructor Iface_get(lua_State *L) {
  const char *iface_name = checkS(L,1);
  Iface* iface = (Iface*) co_iface_get(iface_name);
  if (iface) {
    pushIface(L,iface);
    R1;
  } else {
    return 0;
  }
}

/** @constructor Iface.add(name,family)
  * @brief add an interface by name
  * @param name: the name of the interface
  * @param family: AF_INET or AF_INET6 defaults to AF_INET
  */
Constructor Iface_add(lua_State *L) {
  const char *iface_name = checkS(L,1);
// XXX remove if nil...
  int family = (int)luaL_optnumber(L,2,(luaNumber)(AF_INET));
  Iface* iface = (Iface*) co_iface_add(iface_name, family);

  if (iface) {
    pushIface(L,iface);
    R1;
  } else {
    return 0;
  }
}

// 2do: int co_generate_ip(const char *base, const char *genmask, const nodeid_t id, char *output, int type);

static const luaL_Reg Iface_methods[] = {
    {"get", Iface_get},
    {"add", Iface_add},
    {"iter", Iface_iter},
    {"foreach", Iface_foreach},
    {"wpa_connect", Iface_wpa_connect},
    {"wpa_disconnect", Iface_wpa_disconnect},
    {"set_ip", Iface_set_ip},
    {"unset_ip", Iface_unset_ip},
    {"set_ssid", Iface_set_ssid},
    {"set_bssid", Iface_set_bssid},
    {"set_frequency", Iface_set_frequency},
    {"set_encription", Iface_set_encription},
    {"set_key", Iface_set_key},
    {"set_mode", Iface_set_mode},
    {"set_apscan", Iface_set_apscan},
    {"wireless_enable", Iface_wireless_enable},
    {"wireless_disable", Iface_wireless_disable},
    {"set_dns", Iface_set_dns},
    {"get_mac", Iface_get_mac},
    {"status", Iface_status},
    {"wpa_id", Iface_wpa_id},
    {"wireless", Iface_wireless},
    { NULL, NULL }
};


/*! @class Timer 
 */

/** @method timer.add()
  */           
Method Timer_add(lua_State*L) {
    co_obj_t *o = checkTimer(L,1);
    int r = co_loop_add_timer(o, NULL);
    lua_pushnumber(r);
R1;
}

/** @method timer.remove()
  */           
Method Timer_remove(lua_State*L) {
    co_obj_t *o = checkTimer(L,1);
    int r = co_loop_remove_timer(o, NULL);
    lua_pushnumber(r);
    R1;
}


/** @method timer.set(milis)
  */           
Method Timer_set(lua_State*L) {
    co_obj_t *t = checkTimer(L,1);
    long m = (long)luaL_checknumber(L,2);
    int r = co_loop_set_timer(t,m,NULL);
    R1;
}

           
/** @constructor Timer.get(timer_id)
  */
Constructor Timer_get(lua_State*L) {
    char* timer_id = checkS(L,1); // BUG: 2do  make sure this key persists (anchor to a registry variable?)
    pushTimer(co_loop_get_timer(timer_id,NULL));
    R1;
}

               
/** @constructor Timer.create(timer_id,time)
  */           
Constructor Timer_create(lua_State* L) {
    struct timeval deadline;
    const char* timer_id = checkS(L,1); // this works because in lua strings are unique
                                            // but it needs not to be __gc d 
    double tv = luaL_checknumber(L,2);
    luaL_checktype (L, 3, LUA_TFUNCTION);
    
    ++timer_id;
    deadline.tv_sec = (long)floor(tv);
    deadline.tv_usec = (long)floor((tv - deadline.tv_sec)*1000000.00);
    lua_setfield (L, LUA_REGISTRYINDEX, timer_id);
    pushTimer(co_timer_create(deadline, luawrap_co_cb, (void*)timer_id));
    R1;
}
           
/** @method timer.__tostring()
  */           
MetaMethod Timer__tostring(lua_State* L) {
    co_timer_t* t = checkTimer(L,1);
    pushS(L,(char*)t->ptr);
    R1;
}

static const luaL_Reg Timer_methods[] = {
    {"get",Timer_get},
    {"create",Timer_create},
    {"remove",Timer_remove},
    {"set",Timer_set},
    {"remove",Timer_remove},
    {"add",Timer_add},
    { NULL, NULL }
};

static const luaL_Reg Timer_meta[] = {
    {"__tostring", Timer__tostring},
    { NULL, NULL }
};

/****************************************************************/
/*! @class Process
 */

/* Process callbacks */
DefSelfCb_N(Process,init,name);
DefSelfCb_N(Process,destroy,name);
DefSelfCb_N(Process,stop,name);
DefSelfCb_N(Process,restart,name);
    
static int proc_start(co_obj_t *self, char *argv[]) {
    int i = 0;
    
    lua_getfield(L, LUA_REGISTRYINDEX, SelfCbKey("Process","start",(Process*)->name));
    lua_settop(L,1);
    pushProcess(L,self);
    pushT(L);
    
    for (;*(argv);argv++) {
        pushN(L,++i);
        pushS(L,*argv);
        lua_rawset(L, 3);
    }
    
    if(lua_pcall(L,2,1)) {
        //2do: error
        return 0;
    } else 
        return toN(L,-1,int);
}

/** @constructor process.create(name, pid_file, exec_path, run_path, prof)  
  * @param prof a table for callbacks: init, destroy, start, stop, restart
  * all callbacks are passed the process object and return an integer (0 on success, else is fail)
  * start callback takes also a table of strings as second argument 
  */
Constructor Process_create(lua_State* L) {
    co_process_t proto = {
        {  CO_FLAG_LUAGC,NULL,NULL,0,_ext8 },
        _process, sizeof(co_process_t), 0, false, false, _STARTING,
        "LuaProc", "./pidfile", "./", "./", 0, 0,
        proc_init, proc_destroy, proc_start, proc_stop, proc_restart
    };

    static const char* fn_names[] = {"init","destroy","start","stop","restart",NULL};
    const char* fn_name;
    co_process_t* p;

    proto.name = checkS(L,1);
    proto.pid_file = checkS(L,2);
    proto.exec_path = checkS(L,3);
    proto.run_path = checkS(L,4);
    checkT(L,5);
    
    for (fn_name = (char*)fn_names; *fn_name; fn_name++) {
        pushS(fn_name);
        lua_gettable(L, 5);
        
        if (isF(L,-1))
            lua_setfield(L,LUA_REGISTRYINDEX, SelfCbKey("Process",name,fn_name));
        else {
            // 2do error...
        }
    }

    p = co_process_create(sizeof(lua_process), proto, proto.name, proto.pid_file, proto.exec_path, proto.run_path);
    
    if (p) 
        pushProcess(L,p);
    else
        lua_pushnil(L);
    
    R1;
}

/** @method process.start(argv)
  * @param argv an optional table containing the string arguments to the start procedure
  */
Method Process_start(lua_State* L) {
    co_obj_t *self = checkProcess(L,1);
    char* argv[255];
    argv[0]=NULL;
    
    if (lua_type(L,2) == LUA_TTABLE) {
        lua_pushnil(L);
        while (lua_next(L, 2) != 0) {
            if (i>255) break;
            argv[i] = toS(L, -1);
            lua_pop(L, 1);
            if (argv[i] == NULL) break;
            argv[++i] = NULL;
        }
    }
    
    pushN(L,co_process_start(self,argv));
    R1;
}

/** @method process.__tostring()
  */
Method Process__tostring(lua_State* L) {
    pushFmtS(("Process:%s", checkProcess(L,1)->name ));
    R1;
}

/** @method process.add()
  */
Method Process_add(lua_State*L) {
    co_obj_t *proc = checkProcess(L,1);
    co_obj_unsetflags(proc,CO_FLAG_LUAGC);
    int pid = co_loop_add_process(proc);
    lua_pushnumber(L,(luaNumber)pid);
    R1;
}

/** @fn Process.remove(pid)
  */
Function Process_remove(lua_State*L) {
    int pid = (int)luaL_checknumber(L,1);
    int res = co_loop_remove_process(pid);
    lua_pushnumber(res);
    R1;
}

/** @method process.destroy()
  */
Method_N(Process,destroy,co_process);

/** @method process.stop()
  */
Method_N(Process,stop,co_process);

/** @method process.restart()
  */
Method_N(Process,restart,co_process);

/** @method process.name()
  */
DefRead_S(Process,name);

/** @method process.pid_file()
  */
DefRead_S(Process,pid_file);

/** @method process.exec_path()
  */
DefRead_S(Process,exec_path);

/** @method process.run_path()
  */
DefRead_S(Process,run_path);

static const luaL_Reg Process_methods[] = {
    {"add",Process_add},
    {"remove",Process_remove},
    {"pid_file",Process_pid_file},
    {"exec_path",Process_exec_path},
    {"run_path",Process_run_path},
    {"create",Process_create},
    {"destroy",Process_destroy},
    {"start",Process_start},
    {"restart",Process_restart},
    {"stop",Process_stop},
    { NULL, NULL }
};

static const luaL_Reg Process_meta[] = {
    {"__call", Process_start},
    {"__tostring", Process__tostring},
    {"__gc",gcProcess},
    { NULL, NULL }
};




/*! @class Socket
 */
DefGC(Socket);

/** @method socket.add()
  */
Method Socket_add(lua_State*L) {
    co_obj_t *o = checkSocket(L,1);
    int r = co_loop_add_socket(o, NULL);
    unmarkSocket(o);
    pushN(r);
    R1;
}

/** @method socket.remove()
  */
Method Socket_remove(lua_State*L) {
    co_obj_t *o = checkSocket(L,1);
    int r = co_loop_remove_socket(o, NULL);
    markSocket(o);
    pushN(r);
    R1;
}

/** @constructror Socket.get(uri)
  */
Constructor Socket_get(lua_State*L) {
    char *uri = checkS(L,1);
    co_obj_t *o = co_loop_get_socket(uri, NULL);
    pushSocket(L,o);
    R1;
}

DefSelfCb_N(Socket,init,uri);
DefSelfCb_N(Socket,destroy,uri);
DefSelfCb_N__S(Socket,bind,uri,int);
DefSelfCb_N__S(Socket,connect,uri,int);
DefSelfCb_N__B(Socket,send,uri,int);
DefSelfCb_L__O(Socket,receive,uri,FileDes);
DefSelfCb_N__Oo(Socket,hangup,uri,Obj,NULL);
DefSelfCb_N__Oo(Socket,poll,uri,Obj,NULL);
DefSelfCb_N__Oo(Socket,register,uri,Obj,NULL);
DefSelfCb_N__L_N_N(Socket,setopt,uri);
DefSelfCb_L__N_N(Socket,getopt,uri);

/** @constructror Socket.create(uri, listen, proto)
 * @brief creates a socket from specified values or initializes defaults
 * @param uri uri describing the socket
 * @param listen (defaults to FALSE) 
 * @param proto a table with the callbacks to be invoked
 * callback functions: 
 *  status_int = init(socket) 
 *  status_int = destroy(socket) 
 *  status_int = bind(socket,opt_ctx) 
 *  status_int = connect(socket,opt_ctx) 
 *  status_int = hangup(socket,opt_ctx)
 *  status_int = send(socket,data_buf)
 *  received_buf = receive(socket,fd_int)
 *  status_int = setopt(socket,level_int,option_id_int,optval_buf): 
 *  optval_buf = getopt(socket,level_int,option_id_int): 
 *  status_int = register(socket,opt_ctx): 
*/
Method Socket_create(uri,listen){
    static const char* fn_names[] = {"init","destroy","hangup","bind","connect","send","setopt","getopt","register",NULL};
    const char* fn_name;
    Socket* s;
    Socket proto = {
        {CO_FLAG_LUAGC,NULL,NULL,0,_ext8},_sock,sizeof(Socket),
        "URI",NULL,NULL,FALSE,NULL,NULL,FALSE,
        Socket_init_cb, Socket_destroy_cb, Socket_hangup_cb,
        Socket_bind_cb, Socket_connect_cb, Socket_send_cb, Socket_setopt_cb, Socket_setopt_cb,
        Socket_getopt_cb, Socket_register_cb,
        0
    };

    proto.uri = checkS(L,1);
    proto.listen = optB(L,2,0);
    
    checkT(L,3);
    
    for (fn_name = fn_names; *fn_name; fn_name++) {
        pushS(fn_name);
        lua_gettable(L, 3);
        
        if (isF(L,-1))
            lua_setfield(L,LUA_REGISTRYINDEX, SelfCbKey("Socket",proto.uri,fn_name));
        else {
            // 2do error...
            lua_pop(L,1);
       }
    }

    pushSocket(L,co_socket_create(sizeof(Socket), proto));
    R1;
}

/** @method socket.init()
 * @brief creates a socket from specified values or initializes defaults
 */
DefMethod_N(Socket,init,co_socket,int);

/** @method socket.hangup(opt_context)
 * @brief closes a socket and changes its state information
 * @brief opt_context unused
 */
DefMethod_N__Oo(Socket,hangup,co_socket,int,Obj,NULL);

/** @method socket.send(buf)
 * @brief sends a message on a specified socket
 * @param buf to be sent
 * @param outgoing message to be sent
 * @param length length of message
 */
DefMethod_N_L(Socket,send,co_socket,int);

/** @method socket.receive()
 * @brief receives a message on the listening socket
 */
// 2do: int co_socket_receive(co_obj_t *self, char *outgoing, size_t length);

/** @methos socket.setopt(level,option,value)
 * @brief sets custom socket options, if specified by user
 * @param level the networking level to be customized
 * @param option the option to be changed
 * @param value the value for the new option
 */
DefMethod_N__N_N_L(Socket,setopt,co_socket,int,int,int);

/** @method socket.getopt()
 * @brief gets custom socket options specified from the user
 * @param level the networking level to be customized
 * @param option the option to be changed
 */
DefMethod_L__N_N(Socket,setopt,co_socket,int,int);

/** @method socket.__gc()
 * @brief closes a socket and removes it from memory
 */
Method_N(Socket,destroy,co_socket);


/*! @class UnixSocket 
 */
// dealt with a str8

/** @constructor UnixSocket.unix(name)
 * @brief initializes a unix socket
 * @param self socket name
 */
Constructor UnixSocket__unix(lua_State* L) {
    size_t nlen;
    const char* name = checkL(L,1,&nlen);
    Obj* key = co_str8_create(name, nlen, 0); 
    int r = unix_socket_init(key);
    if (r==0) {
        pushUnixSocket(L,key);
        R1;
    }
}

/** @method unix_socket.bind(endpoint)
 * @brief binds a unix socket to a specified endpoint
 * @param endpoint specified endpoint for socket (file path)
 */
DefMethod_N__S(UnixSocket,bind,unix_socket);

/**
 * @brief connects a socket to specified endpoint
 * @param self socket name
 * @param endpoint specified endpoint for socket (file path)
 */
DefMethod_N__S(UnixSocket,connect,unix_socket);


static const luaL_Reg Socket_methods[] = {
    { NULL, NULL }
};

static const luaL_Reg Socket_metamethods[] = {
    {"__gc",Socket_destroy},
    { NULL, NULL }
};

static const luaL_Reg UnixSocket_methods[] = {
    { NULL, NULL }
};

static const luaL_Reg UnixSocket_metamethods[] = {
    {"__gc",UnixSocket__gc},
    { NULL, NULL }
};




//2do profile.h
//2do tree.h
//2do util.h

// id.h
/** @fn  Sys.node_id_get()
 * @brief Returns nodeid
 */
Function NodeId_get(lua_State* L) {
    nodeid_t id = co_id_get(void);
    pushN(L,id.id);
    R1;
}

// plugin.h
/** @fn Sys.plugin_load()
 * @brief loads all plugins in specified path
 * @param dir_path directory to load plugins from
 */
Function Plugin_load(lua_State* L) {
    pushN(L, co_plugins_load(checkS(L,1)));
    R1;
}


static const luaL_Reg orphan_functions[] = {
    {"node_id_get",NodeId_get},
    {"plugin_load",Plugin_load},
    {NULL,NULL}
}

static void init_globals(lua_State* L) {
    nil_obj = co_nil_create(0);
    gL = L;
}

extern int luawrap_register(lua_State *L) {
    ClassRegister(Cmd);
    ClassRegisterMeta(Cmd);
    ClassRegister(List);
    ClassRegisterMeta(List);
    ClassRegister(Iface);
    ClassRegister(Process);
    ClassRegisterMeta(Process);
    ClassRegister(Timer);
    ClassRegisterMeta(Timer);
    ClassRegisterMeta(Socket);
    ClassRegisterMeta(UnixSocket);
    // 2do AF_INET or AF_INET6
    return 0;

}

#endif
