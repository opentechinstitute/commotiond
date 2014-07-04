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

/*
 * LuaWrapClassDefine(Xxx,check_code,push_code)
 * defines:
 * toXxx(L,idx) gets a Xxx from an index (Lua Error if fails)
 * checkXxx(L,idx) gets a Xxx from an index after calling check_code (No Lua Error if it fails)
 * pushXxx(L,xxx) pushes an Xxx into the stack
 * isXxx(L,idx) tests whether we have an Xxx at idx
 * shiftXxx(L,idx) removes and returns an Xxx from idx only if it has a type of Xxx, returns NULL otherwise
 * LuaWrapClassDefine must be used with a trailing ';'
 * (a dummy typedef is used to be syntactically correct)
 */
#define LuaWrapClassDefine(C,check_code,push_code) \
static C to##C(lua_State* L, int idx) { \
    C* v = (C*)lua_touserdata (L, idx); \
    if (!v) luaL_error(L, "bad argument %d (%s expected, got %s)", idx, #C, lua_typename(L, lua_type(L, idx))); \
    return v ? *v : NULL; \
} \
static C check##C(lua_State* L, int idx) { \
    C* p; \
    luaL_checktype(L,idx,LUA_TUSERDATA); \
    p = (C*)luaL_checkudata(L, idx, #C); \
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
static gboolean is##C(lua_State* L,int i) { \
    void *p; \
    if(!lua_isuserdata(L,i)) return FALSE; \
    p = lua_touserdata(L, i); \
    lua_getfield(L, LUA_REGISTRYINDEX, #C); \
    if (p == NULL || !lua_getmetatable(L, i) || !lua_rawequal(L, -1, -2)) p=NULL; \
    lua_pop(L, 2); \
    return p ? TRUE : FALSE; \
} \
static C shift##C(lua_State* L,int i) { \
    C* p; \
    if(!lua_isuserdata(L,i)) return NULL; \
    p = (C*)lua_touserdata(L, i); \
    lua_getfield(L, LUA_REGISTRYINDEX, #C); \
    if (p == NULL || !lua_getmetatable(L, i) || !lua_rawequal(L, -1, -2)) p=NULL; \
    lua_pop(L, 2); \
    if (p) { lua_remove(L,i); return *p; }\
    else return NULL;\
} \
static int gc#C(lua_State* L) { C* o = to##C(L,1); if ( o && (o->flags & OwnObjFlag) ) co_obj_free(o); return 0; } \
static int dump#C(lua_State* L) { lua_pushstring(L,#C); return 1; } \
typedef int dummy##C

/*
 * Arguments for check_code or push_code
 */
#define FAIL_ON_NULL(s) do { if (! *p) luaL_argerror(L,idx,s); } while(0)
#define CHK_OBJ(Chk,s) do { if ((! *p) || ! Chk(p) ) luaL_argerror(L,idx,s); } while(0)
#define NOP 

/*
 * LuaWrapClassRegister(Xxx)
 */
#define LuaWrapClassRegister(C) { \
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
    lua_pushstring(L, #C); \
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

/*
 * LuaWrapClassRegisterMeta(Xxx)
 */
#define LuaWrapClassRegisterMeta(C) { \
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
    lua_pushstring(L, #C); \
    lua_setfield(L, -2, "__typeof"); \
     /* add the '__gc' metamethod with a C-function named Class__gc */ \
    /* this will force ALL wslua classes to have a Class__gc function defined, which is good */ \
    lua_pushcfunction(L, C ## __gc); \
    lua_setfield(L, -2, "__gc"); \
    /* pop the metatable */ \
    lua_pop(L, 1); \
}

#define LuaWrapFunction static int

#define LuaWrapRegisterFunction(name) do { lua_pushcfunction(L, luawrap_## name); lua_setglobal(L, #name); } while(0);

#define LuaWrapRegister static int

#define LuaWrapMethod static int
#define LuaWrapConstructor static int
#define LuaWrapSet static int
#define LuaWrapGet static int
#define LuaWrapMetaMethod static int
#define LuaWrapRegFn static int

#define LuaWrapMethods static const luaL_Reg
#define LuaWrapMeta static const luaL_Reg

#define LuaWrapError(name,error) do { luaL_error(L, ep_strdup_printf("%s%s", #name ": " ,error) ); return 0; } while(0)
#define LuaWrapArgError(name,idx,error) do { luaL_argerror(L,idx, #name  ": " error); return 0; } while(0)
#define LuaWrapOptArgError(name,idx,error) do { luaL_argerror(L,idx, #name  ": " error); return 0; } while(0)

#define LuaWrapRegGlobalBool(L,n,v) { lua_pushboolean(L,v); lua_setglobal(L,n); }
#define LuaWrapRegGlobalString(L,n,v) { lua_pushstring(L,v); lua_setglobal(L,n); }
#define LuaWrapRegGlobalNumber(L,n,v) { lua_pushnumber(L,v); lua_setglobal(L,n); }

#define LuaWrapReturn(i) return (i);

#define LuaWrapAPI extern


/* Clears or marks references that connects Lua to volatile structures */
#define LuaWrapClearOutstading(C, marker, marker_val) void clear_outstanding_##C(void) { \
    while (outstanding_##C->len) { \
        C p = (C)g_ptr_array_remove_index_fast(outstanding_##C,0); \
        if (p) { \
            if (p->marker != marker_val) \
                p->marker = marker_val; \
            else \
                g_free(p); \
        } \
    } \
}

typedef int (*LuaWrapErrCb)(char* err,LuaWrap** lw);
LuaWrap* luaWrap(char* code,LuaWrapErrCb err_cb);
int luaWrapEval(LuaWrap*, char* code);
void luaWrapFree(LuaWrap*);

// Macros for quick methods

#define OBJ_METHOD_DEF_O(Class,Method,Pre) LuaWrapMethod Class##_##Method(lua_State* L) { \
co_obj_t *o = check##Class(L,1);\
int r = Pre##_##Method(o); lua_pushbool(L,(r!=0)); return 1; }
    
#define OBJ_METHOD_DEF_B__C(Class,Method,Pre) LuaWrapMethod Class##_##Method(lua_State* L) { \
co_obj_t *o = check##Class(L,1); char* s = luaL_checkstring(L,2);\
int r = Pre##_##Method(o,s); lua_pushbool(L,(r!=0)); return 1; }

#define OBJ_METHOD_DEF_B__I(method) LuaWrapMethod Iface_##method(lua_State* L) { \
co_obj_t *o = check##Class(L,1); int i = luaL_checkint(L,2);\
int r = Pre##_##Method(o,i); lua_pushbool(L,(r!=0)); return 1; }

#define OBJ_METHOD_DEF_B__C_C(Class,Method,Pre) LuaWrapMethod Class##_##Method(lua_State* L) { \
co_obj_t *o = check##Class(L,1); char* s1 = luaL_checkstring(L,2); char* s2 = luaL_checkstring(L,3); \
int r = Pre##_##Method(o,s1,s2); lua_pushbool(L,(r!=0)); return 1; }

#define OBJ_METHOD_DEF_B__C_C_C(Class,Method,Pre) LuaWrapMethod Class##_##Method(lua_State* L) { \
co_obj_t *o = check##Class(L,1); char* s1 = luaL_checkstring(L,2); char* s2 = luaL_checkstring(L,3); \
char* s3 = luaL_checkstring(L,4); \
int r = Pre##_##Method(o,s1,s2,s3); lua_pushbool(L,(r!=0)); return 1; }

#define OBJ_METHOD_DEF_B__Cd_Cd(Class,Method,Pre,D1,D2) LuaWrapMethod Class##_##Method(lua_State* L) { \
co_obj_t *o = check##Class(L,1); char* s = luaL_optstring(L,2,D1); char* s2 = luaL_checkoptstring(L,3,D2); \
int r = Pre##_##Method(o,s); lua_pushbool(L,(r!=0)); return 1; }


#define OBJ_METHOD_DEF_GET_BUF(Class,Method,Pre,DefLen) LuaWrapMethod Class##_##Method(lua_State* L) { \
co_obj_t *o = check##Class(L,1); int len = luaL_optint(L,2,DefLen); char* b = malloc(len);\
int r = Pre##_##Method(o,b,len); lua_pushlstring(L,b,len); free(b); return 1; }


#define DEF_OBJ_METH_B__O(C,N,P) LuaWrapMethod  C##_##N(lua_State* L) { co_obj_t* o = check##C(L,1); co_obj_t* i; \
if(!lua_isuserdata(L,2)) i = nil_obj; else i = (co_obj_t*)lua_touserdata (L, 2); \
int res = P##N(o, i); lua_pushboolean(L,res); return 1; }
           
#define DEF_OBJ_METH_O__O(C,N,P) LuaWrapMethod  C##_##N(lua_State* L) { co_obj_t* o = check##C(L,1); co_obj_t* i; \
if(!lua_isuserdata(L,2)) i = nil_obj; else i = (co_obj_t*)lua_touserdata (L, 2); \
co_obj_t* res = P##N(o, i); pushObj(L,res); return 1; }

#define DEF_OBJ_METH_O__O_O(C,N,P) LuaWrapMethod  C##_##N(lua_State* L) { co_obj_t* o = check##C(L,1); co_obj_t* i;  co_obj_t* i2;\
if(!lua_isuserdata(L,2)) i = nil_obj; else i = (co_obj_t*)lua_touserdata (L, 2); \
if(!lua_isuserdata(L,3)) i2 = nil_obj; else i2 = (co_obj_t*)lua_touserdata (L, 3); \
co_obj_t* res = P##N(o, i,i2); pushObj(L,res); return 1; }

#define DEF_OBJ_METH_B__O_O(C,N,P) LuaWrapMethod  C##_##N(lua_State* L) { co_obj_t* o = check##C(L,1); co_obj_t* i;  co_obj_t* i2;\
if(!lua_isuserdata(L,2)) i = nil_obj; else i = (co_obj_t*)lua_touserdata (L, 2); \
if(!lua_isuserdata(L,3)) i2 = nil_obj; else i2 = toObj(L, 3); \
int res = P##N(o, i); lua_pushbool(L,res); return 1; }

#define DEF_OBJ_METH_O__O_I(C,N,P) LuaWrapMethod  C##_##N(lua_State* L) { co_obj_t* o = check##C(L,1); co_obj_t* i; int i2;\
if(!lua_isuserdata(L,2)) i = nil_obj; else i = (C*)lua_touserdata (L, 2); \
co_obj_t* res = P##N(o, i,luaL_checknumber(L, 3)); pushObj(L,res); return 1; }
           
#define LuaWrapRead_S(C,Elem) LuaWrapMethod C##_##Elem(lua_State* L) { \
    C* o = check##C(L,1); lua_pushstring(L,o->Elem); return 1; }

#define LuaWrapRead_N(C,Elem) LuaWrapMethod C##_##Elem(lua_State* L) { \
    C* o = check##C(L,1); lua_pushnumber(L,(lua_Number)o->Elem); return 1; }

#define LuaWrapRead_B(C,Elem) LuaWrapMethod C##_##Elem(lua_State* L) { \
    C* o = check##C(L,1); lua_pushboolean(L,(int)(o->Elem)); return 1; }

#define LuaWrapRead_O(C,Elem,OutC) LuaWrapMethod C##_##Elem(lua_State* L) { \
    C* o = check##C(L,1); push##OutC(L,o->Elem); return 1; }


/*
 * exttypes for lua ojects
 */

#define _luawrap_iterator 0xFE
#define _luawrap_ifaces 0xFD
#define _luawrap_list_iter 0xFC





/*
 * typedefs for Lua classes
 */

typedef co_obj_t Obj;

typedef struct ListIteration {
    co_exttype_t _header;
    co_obj_t* list;
    co_obj_t* next;
} ListIteration;

typedef co_iface_t Iface;
typedef ListIteration Ifaces;
typedef co_cmd_t Cmd;

typedef union List {
    co_list16_t list16;
    co_list32_t list32;
} List;



//2do
typedef union Tree {
    co_tree16_t tree16;
    co_tree32_t tree32;
} Tree;

typedef co_timer_t Timer;
typedef co_plugin_t Plugin;
typedef co_process_t Process;
typedef co_cbptr_t CallBack;
typedef co_profile_t Profile;
typedef co_socket_t Socket;
typedef unix_socket_t USock;
typedef co_fd_t FileDes;
typedef co_obj_t Iterator;

typedef struct NodeId {
    co_exttype_t _header;
    nodeid_t id;
}

typedef struct Sys {
    co_exttype_t _header;
} Sys;


/*
 * Class Definitions
 */

LuaWrapClassDefine(Iface,NOP,NOP);
LuaWrapClassDefine(Ifaces,NOP,NOP);
LuaWrapClassDefine(Cmd,NOP,NOP);

LuaWrapClassDefine(Iterator,NOP,NOP);

LuaWrapClassDefine(List,NOP,NOP);
LuaWrapClassDefine(ListIteration,NOP,NOP);

LuaWrapClassDefine(Timer,NOP,NOP);

//2do
LuaWrapClassDeclare(Plugin);
LuaWrapClassDeclare(Process);
LuaWrapClassDeclare(CallBack);
LuaWrapClassDeclare(Profile);
LuaWrapClassDeclare(Socket);
LuaWrapClassDeclare(USock);
LuaWrapClassDeclare(FileDes);

LuaWrapClassDeclare(Sys);



/*
 *  file globals;
 */
static co_obj_t* nil_obj;


/* Util funcs */

const char* cbkey(const char* a, const char* b) {
    static char key[16][255];
    static int ki = 0;
    static char* k = key[ki++, ki %= 16];
    snprintfcat(k, 255, "%s_%s", a, b);    
    return k;
}



/* Obj, the undeclared class */



static co_obj_t* toObj(lua_State* L, int idx) {
    co_obj_t* o = NULL;
    uint8 o_flags = 0;
    
    switch(lua_type(L,idx)) {
        case LUA_TNUMBER:
            return co_float64_create(lua_tonumber(L,idx),o_flags);
        case LUA_TBOOLEAN:
            return co_bool_create(lua_toboolean(L,idx),o_flags);
        case LUA_TSTRING: do {
                size_t len=0;
                const char * s = lua_tolstring(L, idx, &len);
                return co_str32_create(s,len,o_flags);
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
            o = co_nil_create(o_flags);
            break;
    }           
    return o;
}

#define checkObj(L, idx) (lua_type(L,idx) == LUA_TUSERDATA) ? lua_touserdata(L,idx) : luaL_checkudata(L, idx, "NotAnObject") )

static int gcObj(lua_State* L,int idx) {
    co_obj_t* o = NULL;
    
    if(lua_type(L,idx) == LUA_TUSERDATA)
        o = lua_touserdata(L,idx);
    
    if ( o && (o->flags & OwnObjFlag) ) switch (o->type) {
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
            co_obj_free(o);
            return 0;
        
        case _ext8:
            switch( ((co_ext8_t)o)->_exttype) {
                case _cmd: gcCmd(L,idx); 
                case _iface: gcIface(L,idx);
                case _luawrap_iterator: gcIterator(L,idx);
                case _luawrap_ifaces: gcIfaces(L,idx);
                case _luawrap_list_iter: gcIter(L,idx);
                case _plug: gcPlug(L,idx);
                case _profile: gcProfile(L,idx);
                case _cbptr: gcCallBack(L,idx);
                case _fd: gcFD(L,idx);
                case _sock: gcSocket(L,idx);
                case _co_timer: gcTimer(L,idx);
                case _process: gcProcess(L,idx);
                case _ptr: gcPtr(L,idx);
                default: goto error;
            }
        
        case _ext16:
        case _ext32:
        
        default:
        goto error;
    }
    return 0;
}
static int pushObj(lua_State* L, co_obj_t* o) {
#define PUSH_N_DATA(T, L) lua_pushnumber(L,(luaNumber)(((T##L##_t)o)->data))
#define PUSH_C_DATA(T, L) lua_pushlstring(L,(((T##L##_t)o)->len),(((T##L##_t)o)->data))
    
    int len = 0;
    char* data;
    switch (o->type) {
        
        case _nil: lua_pushnil(L); return 1;
        case _true: lua_pushbool(L,1); return 1;
        case _false: lua_pushbool(L,0); return 1;

        case _float32: PUSH_N_DATA(float,32); return 1;
        case _float64: PUSH_N_DATA(float,64); return 1;
        
        case _uint8: PUSH_N_DATA(uint,8); return 1;
        case _uint16: PUSH_N_DATA(uint,16); return 1;
        case _uint32: PUSH_N_DATA(uint,32); return 1;
        
        case _int8: PUSH_N_DATA(int,8); return 1;
        case _int16: PUSH_N_DATA(int,16); return 1;
        case _int32: PUSH_N_DATA(int,32); return 1;

        case _bin8: PUSH_C_DATA(bin, 8); return 1;
        case _bin16: PUSH_C_DATA(bin, 16); return 1;
        case _bin32: PUSH_C_DATA(bin, 32); return 1;
        
        case _str8: PUSH_C_DATA(str, 8); return 1;
        case _str16: PUSH_C_DATA(str, 16); return 1;
        case _str32: PUSH_C_DATA(str, 32); return 1;
        
        case _list16:
        case _list32:
            pushList(L,o);
            return 1;
        
        case _tree16:
        case _tree32:
            pushTree(L,o);
            return 1;
        
        case _ext8:
        case _ext16:
        case _ext32:
            switch( ((co_exttype_t)o)->_exttype) {
                
                case _cmd: pushCmd(L,o); return 1;
                case _iface: pushIface(L,o); return 1;
                
                case _luawrap_iterator: pushIterator(L,o); return 1;
                case _luawrap_ifaces: pushIfaces(L,o); return 1;
                case _luawrap_list_iter: pushListIteration(L,o); return 1;
                    
                case _plug:  pushPlug(L,o); return 1;
                case _profile: pushProfile(L,o); return 1;
                case _cbptr:  pushCallBack(L,o); return 1;
                case _fd:  pushFd(L,o); return 1;
                case _sock: pushSock(L,o); return 1;
                case _co_timer: pushTimer(L,o); return 1;
                case _process: pushProcess(L,o); return 1;
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
        return 1;
}

/*** iter ***/
           

static co_obj_t* luawrap_iter(co_obj_t *data, co_obj_t *current, void *context) {
    Iter* it = (Iter*)context;
    co_obj_t* out;
    
    lua_getfield(L, LUA_REGISTRYINDEX, cbkey("IT",it->name));
    
    pushObj(L,data);
    pushObj(L,current);
    
    if(lua_pcall(L,2,1)){
        char* err = lua_tostring(L,-1);
        lua_pushnil(L);
        return 1;
    }
    
    out = toObj(L,1);
    lua_remove (L, -1);
    return out;
}

                      
/** callback **/
           
int luawrap_co_cb(co_obj_t *self, co_obj_t **output, co_obj_t *params) {
    co_str8_t s = ((co_str8_t)self);
    
    lua_getfield(L, LUA_REGISTRYINDEX, cbkey("CB",(char*)s->data)); // function_name
    
    pushObj(L,params);
    
    if(lua_pcall(L,2,1)){
        char* err = lua_tostring(L,-1);
        lua_pushnil(L);
        return 1;
    }
        
    *output = toObj(L,1);
    lua_pop(L,1);
    return 1;
}

#define luawrap_quick_check(L) do { luaL_checktype(L, 1, LUA_TFUNCTION); lua_settop(L,1); } while(0)

int luawrap_quick_co_cb(co_obj_t *self, co_obj_t **output, co_obj_t *params) {
    co_str8_t s = ((co_str8_t)self);
    
    
    pushObj(L,params);
    if(lua_pcall(L,2,1)){
        char* err = lua_tostring(L,-1);
        lua_pushnil(L);
        return 1;
    }
        
    *output = toObj(L,1);
    lua_pop(L,1);
    return 1;
}




/*! @class Cmd
 *  @brief a mechanism for registering and controlling display of commands
 */

/** @constructor new
 * @brief create a new command
 * @param name: of the command
 * @param usage: of the command
 * @param desc: of the command
 * @param action: function(self,params) to be called when executed
 */
LuaWrapConstrunctor Cmd_new(lua_State* L) {
    size_t nlen, ulen, dlen;
    int o_flags = 0;
    
    const char *name = luaL_checklstring(L,1,&nlen);
    const char *usage = luaL_checklstring(L,2,&ulen);
    const char *desc = luaL_checklstring(L,3,&dlen);
    luaL_checktype(L, 4, LUA_TFUNCTION);
    
    lua_setfield (L, LUA_REGISTRYINDEX, name);
    
    if ( co_cmd_register(name, nlen, usage, ulen, desc, dlen, luawrap_co_cb) != 0) {
        co_obj_t* cmd = co_cmd_get(name, nlen);
        if (cmd) {
            pushCmd(L,cmd);
            return 1;
        }
    }
    
    lua_pushnil(L);
    return 1;
}

/** @method exec
 * @brief executes a command
 * @param params: the parameters to the command
 * returns the output of the excution
 */
LuaWrapMethod Cmd_exec(lua_State* L) {
    co_obj_t* output;
    co_obj_t* key = checkCmd(L,1);
    co_obj_t* param = lua_touserdata(L, 2);
    
    luaL_checktype(L, 2, LUA_TUSERDATA);
    
    if (co_cmd_exec(key, &output, param) != 0) {
        pushObj(L,output);
        return 1;
    } else {
        lua_pushnil(L);
    }
    
    return 1;
}

/** @method usage
 * @brief usage of the command
 */
LuaWrapMethod Cmd_usage(lua_State* L) {
    co_obj_t* cmd = checkCmd(L,1);
    lua_pushstring(L,cmd->usage);
    return 1;
}
           
/** @method desc
 * @brief description of the command
 */
LuaWrapMethod Cmd_desc(lua_State* L) {
    co_obj_t* cmd = checkCmd(L,1);
    lua_pushstring(L,cmd->desc);
    return 1;
}

/** @method hook
 * @brief add a callback to a command
 * @param action: function to be called
 */
LuaWrapMethod Cmd_hook(lua_State* L) {
    char *kstr = NULL;
    co_obj_t* key = checkCmd(L,1);
    size_t klen = co_obj_data(&kstr, key);
    
    luaL_checktype(L, 2, LUA_TFUNCTION);
    lua_setfield (L, LUA_REGISTRYINDEX, name);
    
    lua_pushboolean(L,co_cmd_hook(key, luawrap_co_cb));
    return 1;
}

LuaWrapMetaMethod Cmd__tostring(lua_State* L) {
    char *kstr = NULL;
    co_obj_t* key = checkCmd(L,1);
    size_t klen = co_obj_data(&kstr, key);
    
    lua_pushlstring(L,kstr,klen);
    return 1;
}

/** @method hooks
 * @brief get the hooks list
 */
LuaWrapRead_Ot(Cmd,hooks,Obj);




/** @method process
 * @brief process all registered commands with given iterator
 * @param iter iterator function reference
 * @param context other parameters passed to iterator (a List)
 */
LuaWrapMethod Cmd_process(lua_State* L) {
    co_obj_t* params = shiftList(L,1);

    luawrap_quick_check(L);
    lua_pushnumber(L,(luaNumber)co_cmd_process(luawrap_co_cb_closure, &));
    return 1;
}
           
static const luaL_Reg Cmd_methods[] = {
    {"new", Cmd_new},
    {"exec", Cmd_exec},
    {"usage", Cmd_usage},
    {"desc", Cmd_desc},
    {"hook", Cmd_hook},
    {"hooks", Cmd_hooks},
    { NULL, NULL }
};

static const luaL_Reg Cmd_meta[] = {
    {"__tostring", Cmd__tostring},
    {"__call", Cmd_exec},
    { NULL, NULL }
};

LuaWrapRegFn Cmd_register(lua_State *L) {
    LuaWrapClassRegister(Cmd);
    LuaWrapClassRegisterMeta(Cmd);
    
    return 0;
}

/** List **/
           
LuaWrapClassDefine(List,NOP,NOP);

int List_length(lua_State* L) {
    co_obj_t* list = checkList(L,1);
    lua_push_number(L,co_list_length(list));
    return 1;
}

int List_get_first(lua_State* L) {
    co_obj_t* list = checkList(L,1);
    pushObj(L,co_list_get_first(list));
    return 1;
}

int List_get_last(lua_State* L) {
    co_obj_t* list = checkList(L,1);
    pushObj(L,co_list_get_last(list));
    return 1;
}

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
        lua_remove (L, -1);
        if (!cont) break; 
    }
    
    lua_pushboolean(L,cont);
    return 1;
}

DEF_OBJ_METH_B__O(List,contains,co_list)
DEF_OBJ_METH_O__O(List,delete,co_list)
DEF_OBJ_METH_B__O_O(List,insert_before,co_list)
DEF_OBJ_METH_B__O_O(List,insert_after,co_list)
DEF_OBJ_METH_B__O(List,prepend,co_list)
DEF_OBJ_METH_B__O(List,append,co_list)
DEF_OBJ_METH_O__O_I(List,element,co_list)


LuaWrapMetaMethod List__tostring(lua_State* L) {
    co_obj_t* list = checkList(L,1);
   
    size_t olen = 255;
    char buff[olen];
    
    olen = co_list_raw(buff, olen, list)
    lua_pushlstring(L,buff,olen);
    return 1;
}

LuaWrapConstructor List_import(lua_State* L) {
    size_t ilen=0;
    co_obj_t* list = NULL;
    const char* buff = luaL_checklstring (L, 2, &ilen);
    size_t olen = co_list_import(&list, buff, list);
        
    if (list) pushList(L,list); else lua_pushnil(L);
    return 1;
}

LuaWrapFunction _List_iter(lua_State *L) {
  co_obj_t* list = toList(L,1);
  void* next = lua_touserdata(L,2);
  co_obj_t* item = co_list_foreach(list,&next);
  lua_pop(L,1);
  lua_pushlightuserdata(L, next);

  if (item) {
    pushObj(L,item);
    LuaWrapReturn(1);
  } else {
    LuaWrapReturn(0);
  }
}

LuaWrapMetaMethod List__call (lua_State *L) {
  co_obj_t* list = checkList(L,1);
  lua_pushlightuserdata(L, NULL);
  lua_pushcclosure(L, _List_iter, 2);
  LuaWrapReturn(0);
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
    { NULL, NULL }
};

LuaWrapRegFn List_register(lua_State *L) {
    LuaWrapClassRegister(List);
    LuaWrapClassRegisterMeta(List);
    return 0;
}

           
           
           

/*** Iface ***/

LuaWrapClassDefine(Iface,CHK_OBJ(IS_IFACE,"must be an Iface"),NOP);

LuaWrapFunction luawrap_iface_remove(lua_State* L) {
  const char *iface_name = luaL_optstring(L,1,NULL);
  int r = co_iface_remove(iface_name);
  if (r) LuaWrapReturn(0);

  LuaWrapErr();
}

//int co_generate_ip(const char *base, const char *genmask, const nodeid_t id, char *output, int type);
//char *co_iface_profile(char *iface_name);
//co_obj_t *co_iface_get(char *iface_name);

OBJ_METHOD_DEF(Iface,wpa_connect,co_iface);
OBJ_METHOD_DEF(Iface,wpa_disconnect,co_iface);
OBJ_METHOD_DEF_C_C(Iface,set_ip,co_iface);
OBJ_METHOD_DEF(Iface,unset_ip,co_iface);
OBJ_METHOD_DEF_C(Iface,set_ssid,co_iface);
OBJ_METHOD_DEF_C(Iface,set_bssid,co_iface);
OBJ_METHOD_DEF_I(Iface,set_frequency,co_iface);
OBJ_METHOD_DEF_C(Iface,set_encription,co_iface);
OBJ_METHOD_DEF_C(Iface,set_key,co_iface);
OBJ_METHOD_DEF_C(Iface,set_mode,co_iface);
OBJ_METHOD_DEF_I(Iface,set_apscan,co_iface);
OBJ_METHOD_DEF(Iface,wireless_enable,co_iface);
OBJ_METHOD_DEF(Iface,wireless_disable,co_iface);
OBJ_METHOD_DEF_C_C_C(Iface,set_dns,co_iface);
OBJ_METHOD_DEF_GET_BUF(Iface,get_mac,co_iface,6);

LuaWrapRead_N(Iface,status)
LuaWrapRead_N(Iface,wpa_id)
LuaWrapRead_N(Iface,wireless)
// 2do struct ifreq ifr;
// 2do  struct wpa_ctrl *ctrl;


static const luaL_Reg Iface_methods[] = {
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

LuaWrapRegFn Iface_register(lua_State *L) {
    LuaWrapClassRegister(Iface);
    return 1;
}




/** Loop **/


LuaWrapFunction Sys_add_process(lua_State*L) {
    co_obj_t *proc = checkProc(L,1);
    int pid = co_loop_add_process(proc);
    lua_pushnumber(L,(luaNumber)pid);
    return 1;
}

LuaWrapFunction Sys_remove_process(lua_State*L) {
    int pid = (int)luaL_checknumber(L,1);
    int res = co_loop_remove_process(pid);
    lua_pushnumber(res);
    return 1;
}

LuaWrapFunction Sys_add_socket(lua_State*L) {
    co_obj_t *o = checkSocket(L,1);
    int r = co_loop_add_socket(o, NULL);
    lua_pushnumber(r);
return 1;
}

LuaWrapFunction Sys_remove_socket(lua_State*L) {
    co_obj_t *o = checkSock(L,1);
    int r = co_loop_remove_socket(o, NULL);
    lua_pushnumber(r);
    return 1;
}

LuaWrapFunction Timer_add(lua_State*L) {
    co_obj_t *o = checkTimer(L,1);
    int r = co_loop_add_timer(o, NULL);
    lua_pushnumber(r);
return 1;
}

LuaWrapFunction Timer_remove(lua_State*L) {
    co_obj_t *o = checkTimer(L,1);
    int r = co_loop_remove_timer(o, NULL);
    lua_pushnumber(r);
    return 1;
}


LuaWrapConstructor Sys_get_socket(lua_State*L) {
    char *uri = luaL_checkstring(L,1);
    co_obj_t *o = co_loop_get_socket(uri, NULL);
    
    pushSocket(L,o);
    return 1;
}

LuaWrapConstructor Timer_set(lua_State*L) {
    co_obj_t *t = checkTimer(L,1);
    long m = (long)luaL_checknumber(L,2);
    int r = co_loop_set_timer(t,m,NULL);
    return 1;
}

           
LuaWrapConstructor Timer_get(lua_State*L) {
    char* timer_id = luaL_checkstring(L,1); // BUG: anchored to a constant variable.
    pushTimer(co_loop_get_timer(timer_id,NULL));
    return 1;
}

               
LuaWrapConstructor Timer_create(lua_State* L) {
    struct timeval deadline;
    const char* timer_id = luaL_checkstring(L,1); // this works because in lua strings are unique
                                            // but it needs not to be __gc d 
    double tv = luaL_checknumber(L,2);
    luaL_checktype (L, 3, LUA_TFUNCTION);
    
    ++timer_id;
    deadline.tv_sec = (long)floor(tv);
    deadline.tv_usec = (long)floor((tv - deadline.tv_sec)*1000000.00);
    lua_setfield (L, LUA_REGISTRYINDEX, timer_id);
    pushTimer(co_timer_create(deadline, luawrap_co_cb, (void*)timer_id));
    return 1;
}
           
           
LuaWrapMetaMethod Timer__tostring(lua_State* L) {
    co_timer_t* t = checkTimer(L,1);
    lua_pushstring(L,(char*)t->ptr);
    return 1;
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

LuaWrapRegFn Timer_register(lua_State *L) {
    LuaWrapClassRegister(Timer);
    LuaWrapClassRegisterMeta(Timer);
    
    return 0;

}

/** Process **/

#define SELF_CB(Name) static int Name(co_obj_t *self) { \
    lua_getfield(L, LUA_REGISTRYINDEX,cbkey(((co_process_t*)self)->name),#Name); \
    pushProcess(L,self); if(lua_pcall(L,1,1)){ return 0; } else return (int)lua_tonumber(L,-1); }

SELF_CB(proc_init)
SELF_CB(proc_destroy)
SELF_CB(proc_stop)
SELF_CB(proc_restart)
    
static int proc_start(co_obj_t *self, char *argv[]) {
    int i = 0;
    
    lua_getfield(L, LUA_REGISTRYINDEX, proc_cbkey(name,"proc_start"));
    pushProcess(L,self);
    lua_newtable(L);
    do {
        lua_pushnumber(L,++i);
        lua_pushstring(L,*argv);
        lua_rawset(L, -3);
    } while (*(++argv));
    
    if(lua_pcall(L,2,1)) {
        return 0;
    }
    else 
        return (int)lua_tonumber(L,-1);
}

LuaWrapConstructor Process_create(lua_State* L) {
    co_process_t lua_process = {
        {  LuaWrap_ObjFlags,NULL,NULL,0,_ext8 },
        _process, sizeof(co_process_t), 0, false, false, _STARTING,
        "LuaProc", "./pidfile", "./", "./", 0, 0,
        proc_init, proc_destroy, proc_start, proc_stop, proc_restart
    };

    static const char* names[] = {"","","","","","proc_init","proc_destroy","proc_start","proc_stop","proc_restart"};

    const char *name = luaL_checkstring(L,1);
    const char *pid_file = luaL_checkstring(L,2);
    const char *exec_path = luaL_checkstring(L,3);
    const char *run_path = luaL_checkstring(L,4);
    int i;
    co_process_t* p;
    
    if ((i = lua_gettop(L)) > 9) {
        lua_settop(L, 9);
        i=9;
    }
    
    for (  ; i > 4; i--) if (lua_type (L, i) == LUA_TFUNCTION) {
        lua_setfield(L,LUA_REGISTRYINDEX, proc_cbkey(name,names[i]));
        lua_pop(L,1);
    }
    
    p = co_process_create(sizeof(lua_process), lua_process, name, pid_file, exec_path, run_path);
    
    if (p) pushProcess(L,p); else lua_pushnil(L);
    return 1;
}

LuaWrapMethod Process_destroy(lua_State* L) {
    co_obj_t *self = checkProcess(L,1);
    int r = co_process_destroy(self);
    lua_pushnumber(L,r);
    return 1;
}

LuaWrapMethod Process_start(lua_State* L) {
    co_obj_t *self = checkProcess(L,1);
    char* argv[255];
    argv[0]=NULL;
    
    if (lua_type(L,2) == LUA_TTABLE) {
        lua_pushnil(L);
        while (lua_next(L, 2) != 0) {
            if (i>255) break;
            argv[i] = lua_tostring(L, -1);
            lua_pop(L, 1);
            i++;
        }
    }
    
    lua_pushnumber(L,co_process_start(self,argv));
    return 1;
}



LuaWrapMethod Process_restart(lua_State* L) {
    co_obj_t *self = checkProcess(L,1);
    lua_pushnumber(L,co_process_restart(self));
    return 1;
}

LuaWrapMethod Process_restart(lua_State* L) {
    co_obj_t *self = checkProcess(L,1);
    lua_pushnumber(L,co_process_restart(self));
    return 1;
}

LuaWrapMethod Process_restart(lua_State* L) {
    co_obj_t *self = checkProcess(L,1);
    lua_pushnumber(L,co_process_restart(self));
    return 1;
}

LuaWrapMethod Process_restart(lua_State* L) {
    co_obj_t *self = checkProcess(L,1);
    lua_pushnumber(L,co_process_restart(self));
    return 1;
}

LuaWrapMethod Process_restart(lua_State* L) {
    co_obj_t *self = checkProcess(L,1);
    lua_pushnumber(L,co_process_restart(self));
    return 1;
}

LuaWrapMethod Process__tostring(lua_State* L) {
    co_obj_t *self = checkProcess(L,1);
    char name[255];
    snprintfcat(name, 255, "Process:%s", ((co_process_t*)self)->name);
    lua_pushstring(L,name);
    return 1;
}

LuaWrapRead_C(Process,name)
LuaWrapRead_C(Process,pid_file)
LuaWrapRead_C(Process,exec_path)
LuaWrapRead_C(Process,run_path)

static const luaL_Reg Process_methods[] = {
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
    { NULL, NULL }
};

LuaWrapRegFn Timer_register(lua_State *L) {
    LuaWrapClassRegister(Process);
    LuaWrapClassRegisterMeta(Process);
    
    return 0;
}
/** Socket **/

/*** Ifaces ***/
Ifaces ifaces = { { LuaWrap_ObjFlags,NULL,NULL,0,_ext8 }, NULL, NULL };


LuaWrapMetaMethod Ifaces_get(lua_State *L) {
  Ifaces* ifs = checkIfaces(L,1);
  const char *iface_name = luaL_checkstring(L,1);
  Iface* iface = (Iface*) co_iface_get(iface_name);
  if (iface) {
    pushIface(L,iface);
    LuaWrapReturn(1);
  } else {
    LuaWrapReturn(0);
  }
}

LuaWrapMetaMethod Ifaces_add(lua_State *L) {
  Ifaces* ifs = checkIfaces(L,1);
  const char *iface_name = luaL_checkstring(L,2);
// XXX remove if nil...
  int family = (int)luaL_optnumber(L,3,(luaNumber)(AF_INET));
  Iface* iface = (Iface*) co_iface_add(iface_name, family);

  if (iface) {
    pushIface(L,iface);
    LuaWrapReturn(1);
  } else {
    LuaWrapReturn(0);
  }
}

LuaWrapMetaMethod Ifaces__call (lua_State *L) {
  pushList(L,co_ifaces_list());
  lua_pushlightuserdata(L, NULL);
  lua_pushcclosure(L, _List_iter, 2);
  LuaWrapReturn(0);
}

LuaWrapMetaMethod Ifaces_foreach (lua_State *L) {
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
    return 1;
}

static const luaL_Reg Ifaces_methods[] = {
    {"get", Ifaces__index},
    {"add", Ifaces__newindex},
    {"iter", Ifaces__call},
    {"foreach", Ifaces_foreach},
    { NULL, NULL }
};


LuaWrapRegFn Ifaces_register(lua_State *L) {
    LuaWrapClassRegister(Ifaces);
    LuaWrapClassRegisterMeta(Ifaces);
    pushIfaces(L,ifaces)
    return 0;
}

#endif
