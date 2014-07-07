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
 *   See Copyright notice at bottom of the file
 */

#ifndef LUAWRAP_H
#define LUAWRAP_H

#include <math.h>
#include <stdio.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

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

#define Fn(Pre,Name) Function Pre##_##Name(DL)
#define Fn_(Pre,Name) Function Pre##__##Name(DL)

// Used to allow a semicolon at the end of Def* macros;
// Credit Balint Reczey
#define _End(C,N) typedef int dummy_##C##_##N

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
    luaL_checkstack(L2,"Unable to grow stack\n"); \
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
    lua_pop(L2); \
    return p ? TRUE : FALSE; \
} \
static C shift##C(DLI) { \
    C* p; \
    if(!lua_isuserdata(LI)) return NULL; \
    p = (C*)lua_touserdata(LI); \
    lua_getfield(L, LUA_REGISTRYINDEX, #C); \
    if (p == NULL || !lua_getmetatable(L, i) || !lua_rawequal(L, -1, -2)) p=NULL; \
    lua_pop(L2); \
    if (p) { lua_remove(LI); return *p; }\
    else return NULL;\
} \
static int dump#C(lua_State* L) { pushS(L,#C); R1; } \
static C* opt#C(DLI,dflt) { retrun ( (lua_gettop(L)<idx || isNil(LI)) ? dflt : check##C(LI) )); R0;}}\
_End(C,ClassDefine)


/** defines garbage collector functions for a Class:
 * markXxx(o) mark an object for garbage collection (to be used when pushing own objects to stack)
 * unmarkXxx(o) unmarks an object for garbage collection (to be used when giving away control)
 * method xxx.__gc() a garbage collector method for an Xxx
*/
#define DefGC(C)  \
static int mark#C(C* o) { co_obj_setflags(o,(o->flags)|CO_FLAG_LUAGC); } \
static int unmark#C(C* o) { co_obj_setflags(o,(o->flags)&(0xff^CO_FLAG_LUAGC)); } \
static int C##__gc(lua_State* L) { Obj* o=(Obj*)to##C(L1); if (o&&(o->flags & CO_FLAG_LUAGC)) co_obj_free(o); R0; }\
_End(C,DefGC)


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
    lua_pop (L1); \
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
    lua_pop (L1); \
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
    lua_pop (L1); \
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
    lua_pop(L1); \
}


/* LuaWrapDeclarations and LuaWrapDefinitions
 */

#define LuaWrapDeclarations(M) \
M const char* tmpFmt(const char *f, ...);\
M const char* v2s(VS* vs, int v, int def); \
M int s2v(VS* vs, const char* s, int def); \
M int pushE(lua_State* L, int idx, VS* e); \
_End(LuaWrapDeclarations,_)


#define LuaWrapDefinitions(M) \
M const char* tmpFmt(const char *f, ...) { \
    static char b[ROTBUF_NBUFS][ROTBUF_MAXLEN]; static int i = 0; \
    va_list a; const char* s = buf[++i, i %= ROTBUF_NKEYS]; \
    va_start(a, f); vsnprintf(s, ROTBUF_MAXLEN, f, a); va_end(a); return s;} \
M const char* v2s(VS* vs, int v, int def) \
 { for (;vs->s;vs++) if (vs->v == v) return vs->s; return def; } \
M int s2v(VS* vs, const char* s, int def) \
 { for (;vs->s;vs++) if ( vs->s == s || strcmp(vs->s,s)==0 ) return vs->v; return def; } \
M int pushE(lua_State* L, int idx, VS* e) {return 0; /*2do*/}; \
_End(LuaWrapDefinitions,_)


/****************************************************************/
/* more Lua macros */


/*
 * RegisterFunction(name)
 * registration code for "bare" functions in global namespace
 */
#define RegisterFunction(name) do { lua_pushcfunction(L, luawrap_## name); lua_setglobal(L, #name); } while(0)


/* function type markers  */
#define Register static int
#define Method static int
#define Constructor static int
#define MetaMethod static int
#define Function static int

/* registration */
#define Methods static const luaL_Reg
#define Meta static const luaL_Reg

/* throws an error */
#define Error(error) do { luaL_error(L, error ); return 0; } while(0)

/* throws an error for a bad argument */
#define ArgError(idx,error) do { luaL_argerror(L,idx,error); return 0; } while(0)

/* throws an error for a bad optional argument */
#define OptArgError(idx,error) do { luaL_argerror(L,idx, error); return 0; } while(0)

/* for registering globals */
#define setGlobalB(L,n,v) { pushB(L,v); lua_setglobal(L,n); }
#define setGlobalS(L,n,v) { pushS(L,v); lua_setglobal(L,n); }
#define setGlobalN(L,n,v) { pushN(L,v); lua_setglobal(L,n); }
#define setGlobalO(C,L,n,v) { push##C(L,v); lua_setglobal(L,n); }


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
#define DefMethod_N(C,M,P)  { pushN(L,P##M(check##C(L1))); R1; } _End(C,M)
/* O* P_M(C*) ==> o = c.f() */
#define DefMethod_O(C,M,P,O) _M(C,M) { push##O(L,P##M(check##C(L1))); R1; } _End(C,M)
/* char* P_M(C*) ==> s = c.f() */
#define DefMethod_S(C,M,P) _M(C,M) { pushS(L,P##M(check##C(L1))); R1; } _End(C,M)
/* int P_M(C*,char*,size_t) ==> b = c.f() */
#define DefMethod_L(C,M,P,Sz) _M(C,M) { char b[Sz]; pushL(L,b,P##M(check##C(L1),b,Sz)); R1; } _End(C,M)

/*  T P_M(C*, char*) ==> n = c.f(s) */
#define DefMethod_N__S(C,M,P) _M(C,M) { pushN(L,P##M(check##C(L1),checkS(L2))); R1; } _End(C,M)
/*  T P_M(C*, char*) ==> n = c.f(opt_s) */
#define DefMethod_N__So(C,M,P,D) _M(C,M) { pushN(L,P##M(check##C(L1),optS(L2,D))); R1; } _End(C,M)
/* T P_M(C*, T) ==> n = c.f(n) */
#define DefMethod_N__N(C,M,P,T) _M(C,M) { pushN(L,P##M(check##C(L1),checkN(L2,T))); R1; } _End(C,M)
/* T P_M(C*, T) ==> n = c.f(opt_n) */
#define DefMethod_N__No(C,M,P,T,D) _M(C,M) { pushN(L,P##M(check##C(L1),optN(L2,T,D))); R1; } _End(C,M)
/* T P_M(C*, C2*) ==> n = c.f(o) */
#define DefMethod_N__O(C,N,P,C2) _M(C,M) { pushN(L,P##M(check##C(L1), check##C2(L2))); R1; } _End(C,M)
/* T P_M(C*, C2*) ==> n = c.f(o) */
#define DefMethod_N__Oo(C,N,P,C2,D2) _M(C,M) { pushN(L,P##M(check##C(L1), opt##C2(L2,D2))); R1; } _End(C,M)

/* O* P_M(C*, char*) ==> o = c.f(s) */
#define DefMethod_O__S(C,M,P,O) _M(C,M) { push##O(L,P##M(check##C(L1),checkS(L2))); R1; } _End(C,M)
/* O* P_M(C*, char*) ==> o = c.f(opt_s) */
#define DefMethod_O__So(C,M,P,O) _M(C,M) { push##O(L,P##M(check##C(L1),checkS(L2))); R1; } _End(C,M)
/* O* P_M(C*, T) ==> o = c.f(n) */
#define DefMethod_O__N(C,M,P,O,T) _M(C,M) { push##O(L,P##M(check##C(L1),checkN(L2,T))); R1; } _End(C,M)
/* O* P_M(C*, T) ==> o = c.f(opt_n) */
#define DefMethod_O__No(C,M,P,O,T,D) _M(C,M) { push##O(L,P##M(check##C(L1),optN(L2,T,D))); R1; } _End(C,M)
/* O* P_M(C*, C2*) ==>  o = c.f(o) */
#define DefMethod_N__O(C,N,P,C2) _M(C,M) { push##O(L,P##M(check##C(L1), check##C2(L2))); R1; } _End(C,M)
/* O* P_M(C*, C2*) ==>  o = c.f(opt_o) */
#define DefMethod_O__Oo(C,N,P,O,C2,D) _M(C,M) { push##O(L,P##M(check##C(L1), opt##C2(L2,D))); R1; } _End(C,M)


/* n = c.f(s1,s2) */
#define DefMethod_N__S_S(C,M,P) _M(C,M) { pushN(L,P##M(check##C(L1),checkS(L2),checkS(L3))); R1; } _End(C,M)
/* n = c.f(s1,opt_s2) */
#define DefMethod_N__S_So(C,M,P,D2) _M(C,M) { pushN(L,P##_##M(check##C(L1),checkS(L2),optS(L3,D2))); R1; } _End(C,M)
/* n = c.f(opt_s1,opt_s2)  */
#define DefMethod_N__S_So(C,M,P,D1,D2) _M(C,M) { pushN(L,P##_##M(check##C(L1),optS(L2,D1),optS(L3,D2))); R1; } _End(C,M)


/* n = c.f(s1,s2,s3) */
#define DefMethod_N__S_S_S(C,M,P) _M(C,M) { pushN(L,P##_##M(check##C(L1),checkS(L2),checkS(L3),checkS(L4))); R1; } _End(C,M)



/* T P_N(C*,C1*,C2*) ==> n = c.f(opt_o1,opt_o2) */
#define DefMethod_N__Oo_Oo(C,N,P,O1,O2) _M(C,M) { pushN(L,P##M(check##C(L1),opt##C3(L3), opt##C2(L2))); R1; } _End(C,M)

/* T P_N(C*,C1*,C2*) ==> n = c.f(opt_o1,opt_o2) */
#define DefMethod_N__O_Oo(C,M,P,O1,O2) _M(C,M) { pushN(L,P##M(check##C(L1), check##C2(L2), opt##C3(L3))); R1; } _End(C,M)

/*  O* P_N(C*,C1*,C2*) ==>  o = c.f(opt_o1,opt_o2) */
#define DefMethod_O__Oo_Oo(C,M,P,O,C1,D1,C2,D2) _M(C,M){ push##O(L,P##M(check##C(L1),opt##C1(L2,D1),opt##C2(L3,D2))); R1; } _End(C,M)

/*  O* P_N(C*,C1*,C2*) ==>  o = c.f(o1,opt_o2) */
#define DefMethod_O__O_Oo(C,N,P,O,C1,D1,C2,D2) _M(C,M) { push##O(L,P##N(check##C(L1),check##D1(L2,D1),opt##C2(L3,D2))); R1; } _End(C,M)


/* O* P_N(C*,C2*,T) ==> o = c.f(o,n) */
#define DefMethod_O__O_N(C,M,P,O,C2,T) _M(C,M) { push##O(L,P##M(check##C(L1),check#C2(L2),checkN(L3,T))); R1; } _End(C,M)

#define DefMethod_N_L(C,M,P) _M(C,M) { size_t len; pushN(P##M(check##C(L1), checkL(L2,&len), len); R1; } _End(C,M)

#define DefMethod_L(C,M,P,Sz) _M(C,M) { char b[Sz]; pushL(L,b,P##_##M(check##C(L1),b,Sz)); R1; } _End(C,M)

#define DefMethod_L__O(C,M,P,Sz,O) _M(C,M) { char b[Sz]; pushL(L,b,P##_##M(check##C(L1),check##O(L2),b,Sz)); R1; } _End(C,M)

#define DefMethod_L__N_N(C,M,P,Sz,O,T1,T2) _M(C,M) { char b[Sz]; pushL(L,b,P##_##M(check##C(L1), checkN(L2,T1), checkN(L3,T2), b, Sz))); R1 } _End(C,M)

#define DefMethod_N__N_N_L(C,M,P,) _M(C,M) { size_t len=0; pushN(L,P##_##M(check##C(L1), checkN(L2,T1), checkL(L3,T2), checkL(L4,&len), len)); R1; } _End(C,M)

/* quick methods for struct accessors */

/* Accesses string from struct Elem   */
#define DefAccessorRW_S(C,E) Method C##_##E(lua_State* L) { C* o=check##C(L1); \
    if (isS(L2)) { /*o->E = toS(L2); */ R0; } else { pushS(L,o->E); R1;} } _End(C,E)
#define DefAccessorRO_S(C,E) Method C##_##E(lua_State* L) { pushS(L,check##C(L1)->E); R1; } _End(C,E)

/* Accesses number from struct Elem  */
#define DefAccessorRW_N(C,E,T) Method C##_##E(lua_State* L) { C* o = check##C(L1); \
    if ( isN(L2) ) { o->E = toN(L2,T); R1; } else { pushN(L,o->E); R1;} } _End(C,E)
#define DefAccessorRO_N(C,E,T) Method C##_##E(lua_State* L) { pushN(L,check##C(L1)->E); R1; } _End(C,E)

/* Accesses enum from struct Elem  */
#define DefAccessorRW_E(C,E,Vs,D) Method C##_##E(lua_State* L) { C* o = check##C(L1); \
    if ( isS(L2) ) { o->E = S2V(Vs,toS(L2),D); R1; } else { pushE(L,o->E,Vs); R1;} } _End(C,E)
#define DefAccessorRO_E(C,E,Vs) Method C##_##E(lua_State* L) { pushE(L,check##C(L1)->E,Vs); R1; } _End(C,E)

/* Accesses bool from struct Elem  */
#define DefAccessorRW_B(C,E) Method C##_##E(lua_State* L) { C* o = check##C(L1); \
    if (isB(L2)) { o->E = toB(L2); lua_settop(L2); R1; } else { pushB(L,o->E); R1;} } _End(C,E)
#define DefAccessorRO_B(C,E) Method C##_##E(lua_State* L) { pushB(L,(int)check##C(L1)->E); R1; } _End(C,E)

/* Accesses O from struct Elem  */
#define DefAccessorRW_O(C,E,O) Method C##_##E(lua_State* L) { C* o = check##C(L1); \
    if (is##O(L2)) { o->E = to##O(L2); R1; } else { push##O(L,o->E); R1;} } _End(C,E)
#define DefAccessorRO_O(C,E,O) Method C##_##E(lua_State* L) { push##O(L,check##C(L1)->E); R1; } _End(C,E)

/* */
//#define _CbErr() { return 0; }  //2do
#define _CbPrep(C,N,Id,K) getReg("CB_" #K); checkLF(L,-1);
#define _CbPcall(Nargs,Ret) if(lua_pcall(L,Nargs,1)) _CbErr() else { return Ret; }

#define CbReg(C,N) setReg("CB_" #C "_" #N)
#define cbReg(c,n) setReg(cbkey("CB_",c,n))

#define DefCb(C,N,Id) static void C##_##N() \
    { _CbPrep(C,N,Id);  _CbPcall(0,0,(void)0); } _End(C,N)

#define DefCb___N(C,N,Id,T) static void C##_##N(T n) \
    { _CbPrep(C,N,Id,C##_##N); pushN(L,n); _CbPcall(1,); } _End(C,N)

#define DefCb___S(C,N,Id) static void C##_##N(const char* s) \
    { _CbPrep(C,N,Id,C##_##N); pushS(L,s); _CbPcall(1,); } _End(C,N)

#define DefCb___L(C,N,Id,T,ST) static void C##_##N(T* b,ST len) \
    { _CbPrep(C,N,Id,C##_##N); pushL(L,(void*)b,(size_t)len); _CbPcall(1,); } _End(C,N)

#define DefCb___O(C,N,Id,C2) static void C##_##N(C2* o) \
    { _CbPrep(C,N,Id,C##_##N); push##C2(L,o); _CbPcall(1,); } _End(C,N)

#define DefCb_N(C,N,Id,T,T1) static T C##_##N() \
    { _CbPrep(C,N,Id,C##_##N); _CbPcall(1,toN(L,-1,T)); } _End(C,N)
#define DefCb_S(C,N,Id,T,T1) static const char* C##_##N() \
    { _CbPrep(C,N,Id,C##_##N); _CbPcall(1,toS(L,-1,T)); } _End(C,N)
#define DefCb_O(C,N,Id,Tout,Targ1) static Tout* C##_##N(Targ1* o) \
    { _CbPrep(C,N,Id,C##_##N); _CbPcall(1,to##Tout(L,-1)); } _End(C,N)
#define DefCb_L(C,N,Id,T,ST) static ST C##_##N(T* out, ST max) \
    { ST len; _CbPrep(C,N,Id,C##_##N); /*push...*/ if(lua_pcall(L,1,1)) {_CbErr()} else \
    { memcpy(out,toL(L,-1,&len),len=len>max?max:len); return len; } } _End(C,N)




#define DefSelfCb_N__O(C,N,Id,T,O) static T C##_##N##_cb(co_obj_t *self, co_obj_t *o) {\
    { _CbPrep(C,N,Id,C##_##N); push#C(L,self); push#O(L,o); _CbPcall(1,toN(L,-1,T)); } _End(C,N)

#define DefSelfCb_N__S(C,N,Id,T) static T C##_##N##_cb(co_obj_t *self, const char *s) { \
    { _CbPrep(C,N,Id);  pushS(L,s); _CbPcall(1,toN(L,-1,T)); } _End(C,N)

#define DefSelfCb_N__L(C,N,Id,T) static T C##_##N##_cb(co_obj_t *self, char *s, size_t len) { \
    { _CbPrep(C,N,Id);   pushL(L,s,len); _CbPcall(1,toN(L,-1,T)); } _End(C,N)

#define DefSelfCb_N__O_L(C,N,Id,T,O) static T C##_##N##_cb(co_obj_t *self, co_obj_t *o, char *s, size_t len) { \
    { sCbPrep(C,N,Id);   push##O(L,o); pushL(L,s,len); _CbPcall(1,toN(L,-1,T)); } _End(C,N)

#define DefSelfCb_O(C,N,Id,T,O) static T C##_##N##_cb(co_obj_t *self, co_obj_t *o, co_obj_t *unused) { \
    { _CbPrep(C,N,Id);   push##O(L,o); pushL(L,s,len); _CbPcall(0,to##O(L,-1)); } _End(C,N)

#define DefSelfCb_N__N_N_L(C,N,Id,T,T1,T2,PT,LT) static T C##_##N##_cb(C*o, T1 i1, T2 i2, PT *s, LT len) { \
    { _CbPrep(C,N,Id);  pushN(L,i1,T1);  pushN(L,i2,T2); pushL(L,s,len); _CbPcall(3,toN(L,-1,T)); } _End(C,N)




#endif

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
