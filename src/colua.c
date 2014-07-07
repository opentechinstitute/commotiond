/* vim: set ts=2 expandtab: */
/**
 *       @file  colua.c
 *      @brief  a Commontiond API for the Lua programming Language
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

#include "extern/luawrap.h"

/****************************************************************/
/* Macros for callback wrappers (used by Process and Socket) */
/* C Class 
 * N Name
 * Id Unique Identifier element in struct
 * T type of number
 * O Type of object
 */
#define _CbErr() { return 0; }  //2do
#define _CbPrep(C,N,Id) getReg(SelfCbKey(#C,#N,((C*)self)->Id)); checkLF(L,-1); push##C(L,self)
#define _CbPcall(Nargs,Ret) if(lua_pcall(L,Nargs+1,1)) _sCbErr() else { return (Ret); }

#define SelfCbKey(C,n1,n2) (cbkey(#C,n1,n2)))

#define DefSelfCb_N(C,N,Id,T) static T C##_##N##_cb(co_obj_t *self) \
    { _CbPrep(C,N,Id);  _CbPcall(0,toN(L,-1,T)); } _End(C,N)

#define DefSelfCb_N__O(C,N,Id,T,O) static T C##_##N##_cb(co_obj_t *self, co_obj_t *o) {\
    { _CbPrep(C,N,Id);  push#O(L,o); _CbPcall(1,toN(L,-1,T)); } _End(C,N)

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



LuaWrapDefinitions();

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


#define pushFmtS(ARGS) ( lua_pushstring(L, tmpFmt ARGS ) )
#define cbkey(a, b, c) tmpFmt("%s_%s_%s", a, b, c);
#define cat(a,b) tmpFmt("%s%s", a, b);

static int do_nothing(lua_State* L) { return 0; }

void checkCbs(lua_State* L,int idx,const char* key,const char* name,const char* fn_names[]) {
    void* fn_name;
   
    checkT(L,idx);
    
    for (fn_name = (void*)fn_names; *((const char*)fn_name); fn_name++) {
        pushS(L,(const char*)fn_name);
        lua_gettable(L, idx);
        
        if (! isF(L,-1)) {
            lua_pop(L1);
            lua_pushcfunction(L,do_nothing);
            // 2do error...
        }
        
        setReg(SelfCbKey(key,name,fn_name));
    }
}


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
        case _true: lua_pushbool(L1); R1;
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
    if(lua_pcall(L1,1)){ err = toS(L,-1); return NULL; }
    lua_pop(L1);
    return current;
}

/** callback **/

/* for registered callback */

/* function on top of the stack */
static int luawrap_quick_cb(co_obj_t *self, co_obj_t **output, co_obj_t *params) {
    co_str8_t s = ((co_str8_t)self);
    pushObj(gL,params);
    if(lua_pcall(L1,1)){err=toS(gL,-1); return 0;}
    *output = shiftObj(gL1);
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

    if(lua_pcall(L2,1)){err=toS(gL,-1); return 0;}
    *output = shiftObj(gL1);
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
    const char *name = luaL_checklstring(L1,&nlen);
    const char *usage = luaL_checklstring(L2,&ulen);
    const char *desc = luaL_checklstring(L3,&dlen);
    int r;
    isF(L4);

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
    const char *name = luaL_checklstring(L1,&nlen);
    pushCmd(L,co_cmd_get(name, nlen));
    R1;
}

/** @method exec
 * @brief executes a command
 * @param params: the parameters to the command
 * returns the output of the excution
 */
Method Cmd_exec(lua_State* L) {
    Cmd* c = checkCmd(L1);
    Obj* param = checkObj(L2);
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
    Cmd* c = checkCmd(L1);
    Obj* key = co_str8_create(c->name, strlen(c->name), 0);
    
    luaL_checktype(L2, LUA_TFUNCTION);
    lua_setfield (L, LUA_REGISTRYINDEX, cbkey("Cmd","",c->name));
    
    lua_pushboolean(L,co_cmd_hook(key, luawrap_cb));
    R1;
}

/** @method __tostring
 * @brief returns the name of the command
 */
MetaMethod Cmd__tostring(lua_State* L) {
    Cmd* c = checkCmd(L1);
    pushFmtS(("Cmd:%s",c->name));
    R1;
}

/** @fn process
 * @brief processes all registered commands with given iterator
 * @param iter iterator function reference
 */
Function Cmd_process(lua_State* L) {
    luaL_checkfunction(L1)
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
    co_obj_t* list = checkList(L1);
   
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
    co_obj_t* list = checkList(L1);
    co_obj_t* cur;
    void* cookie = NULL;
    luaL_checkfunction(L2);
    
    while( cur = co_list_foreach(list, &cookie) ) {
        pushObj(cur);
        lua_call(L1,1);
        cont = lua_toboolean(L,-1);
        lua_pop (L1);
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
  co_obj_t* list = checkList(L1);
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
    const char* buff = luaL_checklstring (L2, &ilen);

    size_t olen = co_list_import(&list, buff, list);
        
    if (list) pushList(L,markList(list)); else lua_pushnil(L);
    R1;
}

Function _List_iter(lua_State *L) {
  co_obj_t* list = toList(L1);
  void* next = lua_touserdata(L2);
  co_obj_t* item = co_list_foreach(list,&next);
  lua_pop(L1);
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
        lua_call(L1,1);
        cont = lua_toboolean(L,-1);
        lua_pop(L1);
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
  const char *iface_name = checkS(L1);
  lua_pushnumber(L,co_iface_remove(iface_name));
  R1;
}

/** @fn Iface.profile(iface_name)
  * @brief remove an interface
  * @param iface_name: the name of the interface to be removed
  */
Function luawrap_iface_profile(lua_State* L) {
  const char *iface_name = checkS(L1);
  pushS(L,co_iface_profile(iface_name));
  R1;
}

/** @constructor Iface.get(name)
  * @brief get an interface by name
  * @param name: iface_name the name of the interface
  */
Constructor Iface_get(lua_State *L) {
  const char *iface_name = checkS(L1);
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
  const char *iface_name = checkS(L1);
// XXX remove if nil...
  int family = (int)luaL_optnumber(L2,(luaNumber)(AF_INET));
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
    co_obj_t *o = checkTimer(L1);
    int r = co_loop_add_timer(o, NULL);
    lua_pushnumber(r);
R1;
}

/** @method timer.remove()
  */           
Method Timer_remove(lua_State*L) {
    co_obj_t *o = checkTimer(L1);
    int r = co_loop_remove_timer(o, NULL);
    lua_pushnumber(r);
    R1;
}


/** @method timer.set(milis)
  */           
Method Timer_set(lua_State*L) {
    co_obj_t *t = checkTimer(L1);
    long m = (long)luaL_checknumber(L2);
    int r = co_loop_set_timer(t,m,NULL);
    R1;
}

           
/** @constructor Timer.get(timer_id)
  */
Constructor Timer_get(lua_State*L) {
    char* timer_id = checkS(L1); // BUG: 2do  make sure this key persists (anchor to a registry variable?)
    pushTimer(co_loop_get_timer(timer_id,NULL));
    R1;
}

               
/** @constructor Timer.create(timer_id,time)
  */           
Constructor Timer_create(lua_State* L) {
    struct timeval deadline;
    const char* timer_id = checkS(L1); // this works because in lua strings are unique
                                            // but it needs not to be __gc d 
    double tv = luaL_checknumber(L2);
    luaL_checktype (L3, LUA_TFUNCTION);
    
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
    co_timer_t* t = checkTimer(L1);
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
    
    getReg(SelfCbKey("Process","start",(Process*)->name));
    lua_settop(L1);
    pushProcess(L,self);
    pushT(L);
    
    for (;*(argv);argv++) {
        pushN(L,++i);
        pushS(L,*argv);
        lua_rawset(L, 3);
    }
    
    if(lua_pcall(L2,1)) {
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
    co_process_t* p;

    proto.name = checkS(L1);
    proto.pid_file = checkS(L2);
    proto.exec_path = checkS(L3);
    proto.run_path = checkS(L4);
    checkCbs(L,5,"Process",name,fn_names);
    
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
    co_obj_t *self = checkProcess(L1);
    char* argv[255];
    argv[0]=NULL;
    
    if (lua_type(L2) == LUA_TTABLE) {
        lua_pushnil(L);
        while (lua_next(L2) != 0) {
            if (i>255) break;
            argv[i] = toS(L, -1);
            lua_pop(L1);
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
    pushFmtS(("Process:%s", checkProcess(L1)->name ));
    R1;
}

/** @method process.add()
  */
Method Process_add(lua_State*L) {
    co_obj_t *proc = checkProcess(L1);
    co_obj_unsetflags(proc,CO_FLAG_LUAGC);
    int pid = co_loop_add_process(proc);
    lua_pushnumber(L,(luaNumber)pid);
    R1;
}

/** @fn Process.remove(pid)
  */
Function Process_remove(lua_State*L) {
    int pid = (int)luaL_checknumber(L1);
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
    co_obj_t *o = checkSocket(L1);
    int r = co_loop_add_socket(o, NULL);
    unmarkSocket(o);
    pushN(r);
    R1;
}

/** @method socket.remove()
  */
Method Socket_remove(lua_State*L) {
    co_obj_t *o = checkSocket(L1);
    int r = co_loop_remove_socket(o, NULL);
    markSocket(o);
    pushN(r);
    R1;
}

/** @constructror Socket.get(uri)
  */
Constructor Socket_get(lua_State*L) {
    char *uri = checkS(L1);
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

    proto.uri = checkS(L1);
    proto.listen = optB(L2,0);
    
    checkCbs(L,5,"Socket",proto.uri,fn_names);
    
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
    const char* name = checkL(L1,&nlen);
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
    pushN(L, co_plugins_load(checkS(L1)));
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
