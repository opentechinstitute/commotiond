--!       @file  luawrap.lua
--      @brief  An API wrapper for the Lua programming Language
--
--     @author  Luis E. Garcia Ontanon <luis@ontanon.org>
--
--     Created  01/07/2014
--    Revision  $Id:  $
--    Compiler  gcc/g++
--   Copyright  Copyright (c) 2014, Luis E. Garcia Ontanon <luis@ontanon.org>
--
--   See Copyright notice at bottom of the file
do
    local lw_include="i.h"
    local lw_code="i.c"
    
    local verbose = 6 -- I'm the verbosity level
    local own_prefix  = arg[0]:match("^(.*/)[%a_.]") or '' -- I hold the own prefix to the script (and its files)
    local module_name

    -- shortcuts
    local E = error -- the error function (will likely change at each line read)
    local f = string.format
    string.f = string.format
    local function EF(...) return E(f(...)) end
    local M = string.match
    string.M = M
    
    function table.top(t) return t[#t] end
    function table.shift(t) return t:remove(1) end
    table.pop = table.remove
    table.push = table.insert
    
    local lw_cmd = {}; --  //LW: commands execution
    local mode = 'static'

    local function dup_to(t) -- I create tables that will copy to the given table (err if key exists)
        -- used to guarantee uniqueness of names between different objects
        return setmetatable({},{
            __newindex=function(o,k,v)
                local e = t[k];
                if e then
                    EF("duplicate name: %s previously used in %s:%d",k, o.filename, o.ln)
                end
                rawset(o,k,v);
                t[k]=v;
            end, 
            __index=rawget,
        })
    end
    
    local types = {}; -- holds all named types (classes, enums, callbacks, functions)
    local prototypes = {} -- I hold the collection of prototypes (by name)
    local classes = dup_to(types) -- I hold the collection of classes (by name)
    local functions = dup_to(types) -- I hold the collection of functions that are not in a class (by name)
    local enums = dup_to(types) -- I hold the enumerations (by name) -- nothing for now
    local callbacks = dup_to(types) -- I hold the callbacks (by name) -- nothing for now

    local test -- I'm a dummy for: test = cond or error()
    
    -- utility stuff
    
    function string.mcount(s,pat) -- I count occurrences of pat in a string
        local ln = 0

        for m in s:gmatch(pat) do
            ln = ln + 1
        end
        
        return ln
    end
    
    function string.split(s,p,pm,pq) -- I somehow split strings
        local t = {}
        for i in (s..(pm or p)):gmatch( f("([^%s]*)%s%s",p,p,(pq or '')) ) do
            table.insert(t,i)
        end
        return t
    end
    
    local function fmt_o(s,o) -- I replace %{xxx} with o[xxx] even %{xxx.yyy.zzz} with o[xxx][yyy][zzz]
        local function replace(m)
            if m:match("[.]") then
                local r = o
                print("<--");
                
                for i,el in ipairs(m:split(".",".")) do 
                    print(i,el);
                    r = r[el]
                    test = r or EF("No such element in table: '%s'",m)
                end
                print("-->");
                return r
            else
                return o[m]
            end
        end
        return s:gsub("%%[{]([%a_.-]+)[}]", replace)
    end

    local F = fmt_o
    string.F = fmt_o

    local function xor(a,b) return (a and not b) or (b and not a) end
    

    
    local function v2s(t,i,_s) -- I dump stuff into a string 
        i = i or "";
        local oi = i
        local s = i
        local tt = type(t)
        
        if     tt== 'string' then
            return '"' .. t .. '"' 
        elseif tt== 'number' then
            return tostring(t)
        elseif tt== 'table' then
            s = s .."{ -- " ..tostring(t).."\n";
            i = "  " .. i

            for k,v in pairs(t) do
                local tv = type(v)
                local tk = type(k)

                s = s .. i 

                if tk == 'string' then
                    s = s .. "'"..k.."' ="
                else
                    s = s .. k .." ="
                end

                if tv == 'table' then
                    --_s = _s or {};
                    --for k,v in ipairs(_s) do
                    --    if t == v then error("circular reference") end
                    --end
                    --table.insert(_s,t)
                    s = s .. v2s(v,i,_s)
                    --table.remove(_s,t)
                elseif tv == 'string' then
                    s = s .. '"' .. v .. '"'
                else
                    s = s .. tostring(v)
                end
                s = s .. ",\n"
            end
        else
            return '[['..tostring(t)..']]'
        end
        return s .. oi .. "}"
    end

    
    -- LW tools:
    local dump = print
    
    local function log(level, ...) -- I dump objects based on verbosity
      local s = ''
      if (tonumber(level) <= tonumber(verbose)) then
        for i,a in ipairs({...}) do s = s .. v2s(a) .. "  " end
      end
      dump(s)
    end
    
    local function D(o) log(0,o) end

    local function output(f,cmd) -- I  manage an output file adding #line directives when needed
        -- return 3 functions:
        --    add(lines,filename,linenum)
        --    vadd(v_level,lines,filename,linenum)
        --    add(lines,filename,linenum)
        local ff = string.f
        local s = cmd and ff('/* %s: Generated on %s by: "%s" */\n',f,os.date(),cmd); -- the output string
        local l = 3; -- the line number in the output
        local last_f = f -- the last file input was added 
            local function add_fn(_s,_f,_l) -- adds _s to the file from _f:_l
                _f = _f or f -- no source means is unsourced generated output (so we point to the output)
                _l = _l or l
            
                --s =s.."{"..last_f.."}"
                --s =s.."{".._f.."}{".._l.."}"
                local __f
                local __l
                
                if (_f == last_f) then
                    __f = nil
                    __l = _l
                else
                    last_f = _f
                    __f = _f
                    __l = _l
                end
                
                local __s = (__f and ff('#line %d "%s"\n',__l,__f) or '') .. _s
                
                __l = __s:mcount("\n")
                l = l + __l
                s = s .. __s
                return __l
            end
        local function write_fn() -- writes the file as it is
                local fd = io.open(f,'w') or EF("cannot open '%s' for writing",f)
                test = fd:write(s) or EF("cannot write to '%s'",f)
                fd:close()
                return l
            end
        local function vadd_fn(_v,_s,_f,_l)
            return (verbose < _v) and add_fn(_s,_v,_f,_l) or 0
        end
        return add_fn, vadd_fn, write_fn
    end
    
    local function reader(filename,add,vadd,finish) -- I create a file reader
        local f = io.open(filename, "r");
        
        if not f then
            EF("Cannot open file: '%s'", filename)
        end

        local s = f:read("*all")
        f:close()
        
        
        local fr = { finish=finish,  ln=0, filename=filename, add=add, vadd=vadd  }
        local it =s:gmatch("([^\n]*)[\n]?")
        function fr.iter()
            fr.ln = fr.ln+1;
            return it()
        end
        
        return fr
    end

    -- C code generators
    
    local function proto2C_call(m) -- given a method I yield the call code
        local s = F("%{proto.type} _ = %{proto.name}(",m)

        for i,a in pairs(m.proto.argv) do
            s = s ..F("%{name}, ",a)
        end

        s = s:sub(1,-3)

        return s .. ");"
    end
    
    
    
    local farg_t2C = { -- function arguments for each Ts
        N=function(a) 
            test = a.name or EF("N-argument[%d] without a name",a.idx);
            if type(a.default) == "string" and a.default ~= "" then
                return F("  %{type} %{name} = optN(L, %{idx}, %{type}, %{default}); /* arg[%{idx}] %{text} */\n",a)
            else
                return F("  %{type} %{name} = checkN(L,%{idx},%{type}); /* arg[%{idx}] %{text} */\n",a)
            end
        end,
        B=function(a) 
            test = a.name or EF("B-argument[%d] without a name",a.idx);
            if a.default and a.default ~= "" then
                return F("  int %{name} = optB(L, %{idx}, (int)%{default}); /* arg[%{idx}] %{text} */\n",a)
            else
                return F("  int %{name} = checkB(L, %{idx}); /* arg[%{idx}] %{text} */\n",a)
            end
        end,
        S=function(a)
            test = a.name or EF("S-argument[%d] without a name",a.idx);
            if a.default and a.default ~= '' then
                return F("  char* %{name} = optS(L, %{idx},%{default}); /* arg[%{idx}] %{text} */\n",a)
            else
                return F("  char* %{name} = checkS(L, %{idx}); /* arg[%{idx}] %{text} */\n",a)
            end
        end,
        L=function(a)
            test = a.name or EF("L-argument[%d] without a name",a.idx);
            test = a.len_name or EF("L-argument[%d] '%s' without len_name",a.idx,a.name)
            test = a.type or "const char*";            
            a.len_type = a.len_type or 'size_t';
            return F('  %{len_type} %{len_name}; %{type} %{name} = checkL(L, %{idx}, &%{len_name}); /* arg[%{idx}] %{text} */\n',a)
        end,
        X=function(a)
            test = a.name or EF("X-argument[%d] without a name",a.idx);
            test = a.type or EF("X-argument[%d] '%s' without a type",a.idx,a.name);
            test = a.value or EF("X-argument[%d] '%s' without a value",a.idx,a.name);
            
            return F("  %{type} %{name} = %{value};  /* arg[%{idx}] %{text} */\n",a)
        end,
        O=function(a) 
            test = a.name or EF("%s-argument[%d] '%s' without a name",a.type,a.idx);
            
           if a.default and a.default ~= '' then
                return F("  %{type}* %{name} = opt%{type}(L, %{idx}, %{default});  /* arg[%{idx}] %{text} */\n",a)
            else
                return F("  %{type}* %{name} = check%{type}(L, %{idx});  /* arg[%{idx}] %{text} */\n",a)
            end
        end,
        F=function(a)
            local cb = callbacks[a.type] or EF("No such Callback '%s'",a.type);
            
            if cb.mode == 'key' then
                test = a.key_name or EF("Keyed Callback '%s' without key_name",a.type)
                
                return F("  %{type} %{name} = check%{type}(L, %{idx}, %{key_name});  /* arg[%{idx}] %{text} */\n",a)
            else
                return F("  %{type} %{name} = check%{type}(L, %{idx});  /* arg[%{idx}] %{text} */\n",a)
            end    
        end,
        K=function(a)
            test =  a.name or EF("K-argument[%d] '%s' without a name",a.idx);
            return F("  %{type} %{name} = checkF(L, %{idx}, %{cname});  /* arg[%{idx}] %{text} */\n",a)
        end,
    }
        
    
    local fret_t2C = { -- function returs for Ts
        N=function(r,m)
            test = r.name or EF("N-return[%d] without a name",r.idx);
            return F("  %{type} %{name}; /* ret[%{idx}] %{text} */\n",r),
                F("  pushN(L,%{name});  /* ret[%{idx}] %{text} */\n",r)
        end,
        S=function(r,m)
            test = r.name or EF("S-return[%d] without a name",r.idx);
            return F("  const char* %{name}; /* ret[%{idx}] %{text} */\n",r),
                F("  pushS(L,%{name});  /* ret[%{idx}] %{text} */\n",r)
        end,
        B=function(r,m)
            test = r.name or EF("B-return[%d] without a name",r.idx);
            return F("  int %{name}; /* ret[%{idx}] %{text} */\n",r),
                F("  pushB(L,%{name});  /* ret[%{idx}] %{text} */\n",r)
        end,
        L=function(r,m)
            test = r.name or E("L-return without a name");
            test = r.len_name or EF("L-return '%s' without len_name",r.name)
            r.len_type = r.len_type or 'size_t';
            r.bufsize = r.bufsize or 65536;
            
            local len = r.len_name ~= '_'
                and F("%{len_type} %{len_name} = %{bufsize};  /* ret[%{idx}] %{text} */\n",r)
                or ''
            
            return  F("  char %{name}[%{bufsize}];  /* ret[%{idx}] %{text} */\n",r) .. len,
                    F("  pushL(L, %{name}, %{len_name});  /* ret[%{idx}] %{text} */\n",r)
        end,
        O=function(r,m)
            test = r.name or EF("%s-return[%d] without a name",r.type,r.idx);
            return F("  %{type}* %{name};  /* ret[%{idx}] %{text} */\n",r),
                F("  push%{type}(L, %{name});  /* ret[%{idx}] %{text} */\n",r)
        end,
    }
    
    local function function2C(m) -- given a method I yield C code for it
        test = m.name or E("function without a name");
        test = m.proto or EF("function '%s' without a prototype",m.name);

        local s = F("// calling: %{proto.line}\nstatic int %{codename}(lua_State* L) {\n",m)
        local s2 = ''
            
        for idx,arg in pairs(m.args) do
            if arg.name ~= '_' then
                s2 = s2 .. farg_t2C[arg.t](arg,m)
            end
        end
        
        s2 = s2 .. "  " .. proto2C_call(m) .. "\n"

        for idx,ret in pairs(m.rets) do
            local _d, _s = fret_t2C[ret.t](ret,m)
            
            if ret.name ~= '_' then
                s = s .. _d
            end
            
            s2 = s2 .. _s

        end

        return f("%s%s  return %d;\n}\n",s,s2,#m.rets)
    end

    local function class2C(c) -- given a class I yield C code for its basic functions or declarations
        return F("typedef %{type} %{name};\n"..((c.own and "LwDeclareClass(%{name});\n") or "LwDefineClass(%{name});\n"),c)
    end

    local function registration2C(fn_name) -- I yield C code for registration to Lua of classes and functions 
        local s = f("extern int %s(lua_State* L) {\n",fn_name)
        local b =  "  lua_settop(L,0);\n  lua_newtable(L);\n"

        for n,c in pairs(classes) do
            s = s .. F("  lual_Reg* %{name}_methods = {\n",c)
            for name, m in pairs(c.methods) do
                s = s .. F('    {"%{name}",%{codename}},\n',m)
            end
            s = s .. "    {NULL,NULL}};\n"
                        
            s = s .. F("  lual_Reg* %{name}_metas = {\n",c)
            for name, m in pairs(c.meta) do
                s = s .. F('    {"%{name}",%{codename}},\n',m)
            end
            s = s .. "    {NULL,NULL}};\n"
            
            b = b .. F('  LwClassRegister(%{name},1);\n',c)

        end
        
        s = s .. "  lual_Reg* functions = {\n"
        for name, f in pairs(functions) do
            s = s .. F('    {"%{name}",%{codename}},\n',f)
        end
        s = s .. "    {NULL,NULL}};\n  lual_Reg* f;\n\n"
        b = b .. "  \n  for (f=functions;f->name;f++) {\n" ..
                 "    pushS(L,f->name); lua_pushcfunction(L,f->func); lua_settable(L,1);\n  }\n\n"
        
        return s .. b .. "  return 1;\n}\n"
    end

    local cb_arg_t = {
        B=function(a) return F("  pushB(L,(int)%{name}); /* arg[%{idx}] %{text} */\n",a) end,
        N=function(a) return F("  pushN(L,(luaNumber)%{name}); /* arg[%{idx}] %{text} */\n",a) end,
        O=function(a) return F("  push%{type}(L,%{name}); /* arg[%{idx}] %{text} */\n",a) end,
        F=function(a) return F("  push%{type}(L,%{name}); /* arg[%{idx}] %{text} */\n",a) end,
        S=function(a) return F("  pushS(L,%{name}); /* arg[%{idx}] %{text} */\n",a) end,
        L=function(a) return F("  pushL(L,%{name},(size_t)%{len_name}); /* arg[%{idx}] %{text} */\n",a) end,
        X=function(a) return F("  %{name} = (%{type})%{value}; /* arg[%{idx}] %{text} */\n",a)
        end,
    }
    
    local cb_ret_t = {
        B=function(a)
            return a.name:M('^[%a][%a_]*$') and F("  int %{name};  /* ret[%{idx}] %{text} */\n",a) or "",
                F("    %{name} = toB(L,%{idx}); /* ret[%{idx}] %{text} */\n",a)
        end,
        N=function(a)
            return a.name:M('^[%a][%a_]*$') and F("  %{type} %{name};  /* ret[%{idx}] %{text} */\n",a) or "",
                F("    %{name} = toN(L,%{idx},%{type}); /* ret[%{idx}] %{text} */\n",a)
        end,
        O=function(a)
            return a.name:M('^[%a][%a_]*$') and F("  %{type}* %{name};  /* ret[%{idx}] %{text} */\n",a) or "",
                F("    %{name} = to%{type}(L,%{idx}); /* ret[%{idx}] %{text} */\n",a)
        end,
        S=function(a)
            return a.name:M('^[%a][%a_]*$') and F("  const char* %{name};  /* ret[%{idx}] %{text} */\n",a) or "",
                F("    %{name} = toS(L,%{idx}); /* ret[%{idx}] %{text} */\n",a)
        end,
        L=function(a)
            local len_def, var_def
            
            if (not a.len_name) or a.len_name:M('^[%a][%a_]*$') then
                len_def = '';
            else
                len_def =  F("%{len_type} %{len_name}; ",a)
            end
                        
            return len_def .. F("  /* ret[%{idx}] %{text} */\n",a),
                F("    %{name} = toL(L,%{idx},&(%{len_name})); /* ret[%{idx}] %{text} */\n",a)
        end,
    }
    

    local function cb2C(c) -- given a callback I yield C code for i
        D(c)
        local s = ""
        local call = ""

        if c.mode == 'closure' then
            s = F("static int %{name}__fn_idx;\n",c);
        else
            call = F('  int fn_idx = lua_gettop(L)+1;\n  lw_getCb(L,cbKey%{name}((void*)(%{key_name})));\n',c);
        end
        
        s = s .. F("LW_MODE %{proto.type} call%{name}(%{proto.arglist}) {\n  luaState* L = lwState();",c)
        
        if c.proto.type ~= 'void' then
            s = s .. F("\n  %{proto.type} _ = %{ret_def};\n",c)
        end
        
        local cf = 'LW_MODE %{name} check%{name}(luaState* L, int idx'
        
        if c.mode == 'closure' then
            cf = cf .. F(") {\n",c)
        else
            cf = cf .. F(", void* %{key_name}) {\n",c)
        end
        
        for idx, a in ipairs(c.args) do
            local c = cb_arg_t[a.t] or function(a) return "" end
            call = call .. c(a);
        end
        
        if c.mode == 'closure' then
            call = call .. f("  if((__res = lua_pcall(L, %d, %d, %s__fn_idx)== LUA_OK)){\n",#c.args,#c.rets,c.name)
        else
            call = call .. f("  if((__res = lua_pcall(L, %d, %d, fn_idx)== LUA_OK)){\n",#c.args,#c.rets)
        end
        
        for idx,r in ipairs(c.rets) do
            local c = cb_ret_t[r.t] or EF("return %{name} is of invalid type '%{t}'",r);
            local d, b = c(r);
            
            s = s .. d
            call = call .. b
        end
        
        call  = call .. f("    lua_pop(L,%d)\n",#c.rets)
        
        if c.proto.type ~= 'void' then
            call = call .. F("    return _;\n",c)
        else
            call = call .. F("    return;\n",c)
        end
                
        call = call .. F('  } else lwPcallErr(%{name},__res);\n\n',c)

        if c.proto.type ~= 'void' then
            call = call .. F("  return _;\n}\n",c)
        else
            call = call .. "  return;\n}\n"
        end
        
        cf = cf .. "  checkF(L,idx);\n"
        
        if c.mode == 'closure' then
            cf = cf .. "  %{name}__fn_idx = idx;\n"
        else 
            cf = cf .. '  lw_setCb(L,cbKey%{name}(%{key_name}));\n'
        end
        cf = cf .. "\n return call%{name};\n}\n";
                
        return s .. call  .. F(cf,c);
    end
    
    
    local function splitParams(sep,params,names,ps)
        -- I split a separated list, and assign its elements to an object based on names
        --log(5,'<splitParams',params,names,ps)
        local i = 1
        ps = ps or {}
        for p in (params..sep):gmatch("%s*([^"..sep.."]*)%s*" .. sep) do
            local name = names[i]
            i = i + 1
            if name then ps[name] = p else break end
        end
        --log(5,'splitParams>',v2s(ps))
        return ps, i
    end

    local function splitList(list,t_pars,tbl)
        --log(5,'<splitList',list,v2s(t_pars),v2s(rets))
        tbl = tbl or {}
        local i = 0
        
        for ty, par in list:gmatch("(%u[%a]*)[{]%s*([^}]*)%s*[}]") do
            log(0,"%s = %s",ty,par)
            i = i + 1
            local r = {idx = i, text = f("%s{%s}",ty,par)}
            local t_par = t_pars[ty]
            
            if t_par then
                r.t = ty
            else
                local t = (types[ty] or EF("no such type: '%s'",ty)).t
                r.t = t
                r.type = ty
                t_par = t_pars[t] or EF("No params for type '%s'",t)
            end
            
            splitParams(";",par,t_par,r)
            table.insert(tbl,r)
        end
        
        --log(5,'splitList>',v2s(rets))
        return tbl, i
    end
    


    function lw_cmd.Module(params,val,frame)
        if module_name then E("module already defined") end        
        log(2,'Module:',params)
        local p = params:split("%s"," ","+");
        module_name = p[1]:M("^(%u%a+)$") or E("no module name given")
        mode = M("^(%a+)$",p[1]) or "static"
        test = mode:M("static") or E("only static mode supported")
        local s = "#include <stdio.h>\n#include <lua.h>\n#include <lualib.h>\n#include <lauxlib.h>\n"
          .. f("#define LW_MODULE %s\n#define LW_MODE %s\n",module_name,mode)
        frame.add(s)
        
        
        if mode == 'static' then
            local fn = own_prefix .. lw_include
            local fd = io.open(fn,"r") or E("cannot open: " .. fn)
            frame.add(fd:read("*all") or E("cannot read: " .. fn),fn,1)
            fd:close()


            fn = own_prefix .. lw_code
            fd = io.open(fn,"r") or E("cannot open: " .. fn)
            frame.add(fd:read("*all") or E("cannot read: " .. fn),fn,1)
            fd:close()
        else -- 'extern'
            -- copy luawrap.h
            -- copy luawrap.c
            frame.add('#include "luawrap.h\n')
        end
        
        log(3,'Module:',module_name,mode)
    end

    
    local function class (params,val,frame)
        if not module_name then E("no module defined") end        
        local c = {filename=frame.filename,ln=frame.ln, methods={}, meta = {}}
        local p = params:split("%s"," ","+");
        
        c.name = p[1]:match("^([%a]+)$") or E("Class must have a name [A-Z][A-Za-z0-9]+")
        c.type = p[2]:match("^([%a_]+)$") or E("Class must have a type [A-Za-z0-9]+")
        c.t = 'O';
        classes[c.name] = c

        return c
    end
    
    function lw_cmd.Class(params,val,frame)
        log(2,'Class:',params,val)
        local c = class(params,val,frame)
        log(3,'Class:',c)
        local s = class2C(c)
        frame.add(s);
    end
    
    function lw_cmd.OwnClass(params,val,frame)
        log(2,'OwnClass:',params,val)
        local c = class(params,val,frame)
        c.own = true
        log(3,'OwnClass:',c)
        local s = class2C(c)
        frame.add(s);
    end

    function lw_cmd.Alias(params,val,frame)
       if not module_name then E("no module defined") end        
       log(5,'Alias:',params,val)
        do return end
        local a = {}
        log(3,'Alias:',a)
    end
    
    function lw_cmd.Accessor(params,val,frame)
        if not module_name then E("no module defined") end        
        log(5,'Accessor:',params,val)
        do return end
    end
    
    function lw_cmd.OwnFunction(params,val,frame)
        if not module_name then E("no module defined") end        
        log(5,'OwnFunction:',params,val)
        do return end
    end

    function lw_cmd.Finish(params,val,frame)
        if not module_name then E("no module defined") end        
        log(5,'Finish:',params,val)
        local fname = params:M("^%s*([%a_]+)%s*$") or E("Not a valid registration function name");
        local s = registration2C(fname);
        frame.add(s);
    end
    
    local function shift(t) return table.remove(t,1) end

    

    local arg_t_opts = { -- options for function arguments part
        N={"name","type","default"},
        S={"name","default"},
        B={"name","default"},
        L={"name","len_name","len_type"},
        O={"name","default"},
        X={"name","value","type"},
        F={"name","key_name"},
    }
    
    local ret_t_pars = { -- options for return argument parts
        N={"name","default"},
        S={"name","default"},
        B={"name","default"},
        L={"name","len_name","bufsize","len_type","len_def","default"},
        O={"name","ref","default"},
    }        
    
    function lw_cmd.Function(params,val,frame,m)
        if not module_name then E("no module defined") end        
        log(2,'Function:',params,val)
        local m = m or {
            filename=frame.filename, ln=frame.ln, rets={}, args={}, names={}, params=params, expr=val
        }
        
        local pars = params:split("%s"," ",'+');
        m.fullname = shift(pars) or E("Function/Callback must have name parameter")
        m.fn_name = shift(pars) or E("Function/Callback must have fn_name parameter")        
        m.proto = prototypes[m.fn_name] or E("no prototype found for: " .. v2s(m.fn_name) )
        
        local collection
        if m.is_cb then
            test = m.proto.typedef or EF("Callback prototype must be a callback prototype");
            m.ret_def = shift(pars)
            
            m.name, m.key_name = m.fullname:match("%s*(%u[%a]+)[{]?([%a_]*)[}]?%s*")
            
            test = m.name or EF("CallBack %s has no good name",m.fullname);
            
            m.mode = m.key_name ~= '' and 'key' or 'closure'
            
            if m.proto.type ~= 'void' and not m.ret_def then
                EF("Callback '%s' has non void return type but was given no return default.",m.name)    
            end
            
            collection = callbacks
        else
            if m.fullname:match("%s*(%u[%a]+)[.]([%a_]+)%s*") then
                m.cname, m.name = m.fullname:match("(%u[%a]+)[.]([%a_]+)")
                m.codename = m.cname .. "_" .. m.name
                m.is_meta = m.name:M("^__") and true or false
                local class = classes[m.cname] or E("no such class: " .. v2s(m.cname))

                collection = class[m.is_meta and "meta" or "methods"]
            elseif m.fullname:match("([%a_]+)") then
                m.name = m.fullname
                m.codename = module_name .. "_" .. m.name
                m.is_orphan = true;
                collection = functions
            else
                E("Not a good name: "..v2s(m.fullname))
            end
        end
            
        local i = 0
        local ret, args = val:match("%s*([^@]*)%s*[@]?([^@]*)%s*")
        
        test = (ret and args) or E("malformed method:" .. v2s(m.fn_name) )
        
        D(ret)
        D(args)
        splitList(ret,ret_t_pars,m.rets)
        splitList(args,arg_t_opts,m.args)
        
        for k,a in pairs(m.args) do
            if not a.type and a.t ~= 'K' and a.t ~= 'F'  then
                if m.proto.args[a.name] then
                    a.type = m.proto.args[a.name].type
                else
                    EF("In function %s untyped argument: '%s' is not in prototype",m.fullname,a.name)
                end
            end
        end
                
        collection[m.name] = m
        frame.add((m.is_cb and cb2C or function2C)(m))
        log(3,"Function",m)
    end

    function lw_cmd.CallBack(params,val,frame)
        if not module_name then E("no module defined") end
        
        local cb = lw_cmd.Function(params,val,frame,{
            is_cb=true,filename=frame.filename, ln=frame.ln,
            rets={}, args={}, names={}, params=params, expr=val, t='F'
        })
    end
    
    
    local function lwcmd(lwc,frame,stack)
        local cmd,params,val = lwc:match("(%u[%a]+)%s*([^:]*)[:]?%s*(.*)%s*")
        log(1,"LWD:", lwc,cmd,params,val)
        frame.add(f("// %s:%d #%s\n",frame.filename,frame.ln, lwc))
        if cmd == 'ProcFile' then
            local infile, outfile = lwc:match('ProcFile%s+([^%s]+)%s*([^%s]*)%s*')
            
            test =  not infile and E("No input file in ProcFile directive")
            local new_fr = reader(infile,frame.add,frame.vadd,frame.finish)
            table.insert(stack,new_fr)
            frame = stack[#stack]
            if outfile then
                if mode == 'extern' then
                    frame.add, frame.vadd, frame.finish = output(outfile)
                else
                    E("Cannot use ProcFile in 'static' mode");
                end
            end
            return frame 
        elseif cmd then
            if lw_cmd[cmd] then
                lw_cmd[cmd](params,val,frame)
                return frame;
            else
                EF("No such command: '%s'", cmd)
            end
        else
            EF(" bad cmd: '%s'",lwc)
        end
    end
    
        
        
    local function process(filename,add,vadd,finish)
        local stack = {reader(filename,add,vadd,finish)};
        stack[1].base = true;
        
        while #stack > 0 do
            local frame = stack[#stack]
            local ml_lwc
            
            while true do
                local line = frame.iter()
                if not line then break end
                
                frame.cur_line = line

                if mode=='static' and frame.base then
                    if not (ml_lwc or line:match("^[#][%u]")) then
                        frame.add(line.."\n",frame.filename,frame.ln)
                    end
                end
                
                local function _err(str)
                    local s = f('%s:%d: error: "%s"\nLine:%s\n',frame.filename,frame.ln,str,v2s(line))
                    log(0,s)
                    add(f('#error "%s" ',str),frame.filename,frame.ln);
                    finish();
                    io.stderr:write(s)
                    os.exit(3)
                end
            
                E = _err
                error = _err
                
                if ml_lwc then -- in a multi-line command
                    ml_lwc = ml_lwc ..' '.. line
                    if not line:match("[\\]$") then
                        frame = lwcmd(ml_lwc:gsub("\n",' '),frame,stack)
                        ml_lwc = nil
                    end
                elseif line:match("^[#][%u].*[\\]$") then -- multi-line LuaWrap command start
                    ml_lwc = line:match("^#([%u].*)[\\]$")
                elseif line:match("^[#][%u].*[^\\]$") then -- one line LuaWrap commands
                    frame = lwcmd(line:match("^[#]([%u].*)$"),frame,stack)
                elseif line:match('^[#]include%s+["]([[%a_/.]+[.]h)["]') and frame.base then -- #include
                    local incl = line:match('^[#]include%s+["]([[%a_/.]+[.]h)["]')
                    local new_fr = reader(incl,add,vadd,finish);
                    table.insert(stack,new_fr)
                    frame = stack[#stack]
                elseif line:match("^%a[%a_%s*]*[(].*;%s*$") then -- C function prototypes an typedefs
                    local proto = {
                            args={},argv={},line=line,file=frame.filename,ln=frame.ln
                        }
                    local what
                    
                    if line:match("^typedef ") then
                        local logline = "CB typedef: " .. line
                        log(1,logline)
                        frame.vadd(1,"// " .. logline)
                        
                        line = line:gsub("^typedef%s+","")
                        line = line:gsub("[(]%s*[*]%s*([%a_]+)%s*[)]","%1")
                        proto.typedef = true
                        what = 'typedef'
                    else
                        log(1,"C prototype: " .. line)
                        frame.vadd(1,"// C prototype: " .. line)
                        what = 'prototype'
                    end

                    local i = 0;

                    proto.type, proto.name, proto.arglist =
                        line:match("%s*([%a_]+%s*[%s*]+)([%a_]+)%s*[(](.*)[)]")
                    
                    if not (proto.type and proto.name and proto.arglist ) then
                        E("Cannot fetch %s from line...",what)
                    else
                        for arg in (proto.arglist..','):gmatch("([^,]+),") do
                            i = i + 1
                            local t, name = arg:match("%s*([%a_]*%s*[%a_]*%s*[%a_]+[%s*]+)([%a_]+)%s*")
                            if (t and name) then
                                local a = {}
                                a.type, a.name = t, name;
                                a.idx = i
                                proto.args[a.name]=a
                                table.insert(proto.argv,a)
                            else
                                E("Cannot fetch %s from line...",what)
                            end
                        end
                        log(3,proto)
                        prototypes[proto.name] = proto;
                    end
                end
            end
            if frame.finish then frame.finish() end
            table.remove(stack)
            log(4,"done with file:", frame.filename)
        end
    end
    
    local function usage() 
        print("usage: " .. arg[-1] .." ".. arg[0] .. " [verbosity] <infile> <outfile>")
        os.exit(1)
    end

    if not arg[1] then
        usage()
    end
    
    local fname, add, vadd, finish

    if arg[1]:match('^[%d]+$') then
        verbose = tonumber(arg[1])
        fname = arg[2] or usage()
        if arg[3] then
            add, vadd, finish = output(arg[3],f("%s %s %s %s %s",arg[-1],arg[0],arg[1],arg[2],arg[3]))
        else
            usage()
        end
    else
        fname = arg[1]
        if arg[2] then
            add, vadd, finish = output(arg[2],f("%s %s %s %s",arg[-1],arg[0],arg[1],arg[2]))
        else
            usage()
        end
    end

    local log_fd = {close=function()end}
    
    if (verbose > 0) then 
        local efn = f("%s.log",fname);
        log_fd = io.open(efn,'w') or EF("cannot open file: '%s'",efn)
        dump = function(s) log_fd:write(s .. "\n"); io.stderr:write(s .. "\n"); end
    end
    
    process(fname, add, vadd, finish)
    
    log_fd:close();
    
end 

-- Copyright (C) 2014 Luis E. Garcia Ontanon <luis@ontanon.org>
-- 
-- Permission is hereby granted, free of charge, to any person obtaining a copy of this
-- software and associated documentation files (the "Software"), to deal in the Software
-- without restriction, including without limitation the rights to use, copy, modify,
-- merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
-- permit persons to whom the Software is furnished to do so, subject to the following
-- conditions:
-- 
-- The above copyright notice and this permission notice shall be included in all copies
-- or substantial portions of the Software.
-- 
-- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
-- INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
-- PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
-- HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
-- CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
-- THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
