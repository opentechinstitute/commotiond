--!       @file  luawrap.lua
--      @brief  An API wrapper for the Lua programming Language
--
--     @author  Luis E. Garcia Ontanon <luis@ontanon.org>
--
--     Created  01/07/2014
--    Revision  $Id:  $
--    Compiler  gcc/g++
--   Copyright  Copyright (c) 2013, Luis E. Garcia Ontanon <luis@ontanon.org>
--
--   See Copyright notice at bottom of the file
do
    local lw_include="i.h"
    local lw_code="i.c"
    
    local verbose = 6 -- I'm the verbosity level
    local __err = error -- the error function (will change at each line read)
   
    local function dup_to(t) -- I create tables that will copy to the given table (err if key exists)
        -- used to guarantee uniqueness of names between different objects
        return setmetatable({},{
            __newindex=function(o,k,v)
                if t[k] then
                    local o = t[k];
                    __err(("duplicate name: %s previously used in %s:%d")
                            :format(k, o.filename, o.ln))
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
    local prefix -- I hold the prefix to the script (and its files)
    local module_name

    local test -- I'm a dummy for: test = cond or error()
    
    function table.top(t) return t[#t] end
    function table.shift(t) return t:remove(1) end
    table.pop = table.remove
    table.push = table.insert
    
    
    
    function string.mcount(s,pat) -- I count occurrences of pat in a string
        local ln = 0

        for m in s:gmatch(pat) do
            ln = ln + 1
        end
        
        return ln
    end
    
    local f = string.format
    string.f = string.format
    
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
                
                for i,el in ipairs(m:split(".",".")) do 
                    r = r[el]
                    test = r or __err(f("No such element in table: '%s'",m))
                end
                return type(r) == "string" and r or __err("WTF")
            else
                return o[m]
            end
        end
        return s:gsub("%%[{]([%a_.-]+)[}]", replace)
    end

    local F = fmt_o
    local M = string.match
    string.M = M
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
                    --for k,v in pairs(_s) do
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
    
    local function log(...) -- I dump objects based on verbosity
      local level = table.remove(arg,1);
      local s = ''
      if (level <= verbose) then
        for i,a in ipairs(arg) do s = s .. v2s(a) .. "  " end
      end
      print(s)
    end
    
    local function output(f,cmd) -- I manage an output file adding #line directives when needed
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
                
                local __s = (__f and ff('#line %d %s\n',__l,__f) or '') .. _s
                
                __l = __s:mcount("\n")
                l = l + __l
                s = s .. __s
                return __l
            end
        local function write_fn() -- writes the file as it is
                local fd = io.open(f,'w')
                fd:write(s);
                fd:close()
                return l
            end
        local function vadd_fn(_v,_s,_f,_l)
            return (verbose < _v) and add_fn(_s,_v,_f,_l) or 0
        end
        return add_fn, vadd_fn, write_fn
    end
    
    
    -- C code generators
    
    function proto2C_call(m) -- given a method I yield the call code
        local s = F("%{proto.type} _ = %{proto.name}(",m)

        for i,a in pairs(m.proto.argv) do
            s = s ..F("%{name}, ",a)
        end

        s = s:sub(1,-3)

        return s .. ");"
    end
    
    
    
    local farg_t2C = { -- function arguments for each Ts
        N=function(a) 
            if type(a.default) == "string" and a.default ~= "" then
                return F("  %{type} %{name} = optN(L, %{idx}, %{type}, %{default});\n",a)
            else
                return F("  %{type} %{name} = checkN(L,%{idx},%{type});\n",a)
            end
        end,
        B=function(a) 
            if a.default and a.default ~= "" then
                return F("  int %{name} = optB(L, %{idx}, (int){default});\n",a)
            else
                return F("  int %{name} = checkB(L, %{idx});\n",a)
            end
        end,
        S=function(a)
            if a.default and a.default ~= '' then
                return F("  char* %{name} = optS(L, %{idx},%{default});\n",a)
            else
                return F("  char* %{name} = checkS(L, %{idx});\n",a)
            end
        end,
        L=function(a)
            return F('  %{size_type} %{len_name};\n    %{type} %{name} = checkL(L, %{idx}, &%{len_name});\n',a)
        end,
        X=function(a) return F("  %{type} %{name} = %{value}\n",a)  end,
        O=function(a) 
            if a.default and a.default ~= '' then
                return F("  %{type}* %{name} = opt%{type}(L, %{idx}, %{default});\n",a)
            else
                return F("  %{type}* %{name} = check%{type}(L, %{idx});\n",a)
            end
        end,
        F=function(a)
            return F("  %{type} %{name} = checkF(L, %{idx}, %{cname});\n",a)
        end,
    }
        
    
    local fret_t2C = { -- function returs for Ts
        N=function(r,m) return "",f("  pushN(L,%s);\n",r.name) end,
        S=function(r,m) return "",f("  pushS(L,%s);\n",r.name) end,
        B=function(r,m) return "",f("  pushB(L,%s);\n",r.name) end,
        L=function(r,m)
            return  F("  char %{name}[%{bufsize}];\n  %{size_type} %{len_name} %{len_def};\n",r),
                    F("  pushL(L, %{name}, %{len_name});\n",r)
        end,
        O=function(r,m) return "",F("  push%{type}(L, %{name});\n",r) end,
    }
    
    function function2C(m) -- given a method I yield C code for it
        
        local s = F("// calling: %{proto.line}\nstatic int %{cname}(lua_State* L) {\n",m)
        local s2 = ''
            
        for idx,arg in pairs(m.args) do
            
            s2 = s2 .. farg_t2C[arg.t](arg,m)
        end
        
        s2 = s2 .. proto2C_call(m)

        for idx,ret in pairs(m.rets) do
            local _d,_s = fret_t2C[ret.t](ret,m)
            s = s .. _s
            s2 = s2 .. _s
        end

        return f("%s%s  return %d;\n}\n",s,s2,#m.rets)
    end

    function class2C(c) -- given a class I yield C code for its basic functions or declarations
        return F("typedef %{type} %{name};\n"..((c.own and "LwDeclareClass(%{name});\n") or "LwDefineClass(%{name});\n"),c)
    end

    function registration2C() -- I yield C code for registration to Lua of classes and functions 
        local s = f("extern int %s_register(lua_State* L) {\n",module_name)
        local b =  "  lua_settop(L,0);\n  lua_newtable(L);\n"
        
        for n,c in pairs(classes) do
            s = s .. F("  lual_Reg* %{name}_methods = {\n",c)
            for name, m in c.methods do
                s = s .. F('    {"%{name}",%{cname}},\n',m)
            end
            s = s .. "    {NULL,NULL}\n  };\n"
                        
            s = s .. F("  lual_Reg* %{name}_metas = {\n",c)
            for name, m in c.metas do
                s = s .. F('    {"%{name}",%{cname}},\n',m)
            end
            s = s .. "    {NULL,NULL}\n  };\n"
            
            b = b .. F('  LwClassRegister(%{name},1);\n',c)

        end
        return s .. b .. "  return 1;\n}\n"
    end

    function cb2C(cb) -- given a callback I yield C code for it
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
            i = i + 1
            r = {idx = i}
            local t_par = t_pars[ty]
            
            if t_par then
                r.t = ty
            else
                local t =  (types[ty] or __err("no such type: " .. ty)).t
                r.t = t
                r.type = ty
                t_par = t_pars[t]
            end
            
            splitParams(";",par,t_par,r)
            table.insert(tbl,r)
        end
        
        --log(5,'splitList>',v2s(rets))
        return rets, i
    end
    

    local arg_t_opts = { -- options for function arguments part
        N={"name","type","default"},
        S={"name","default"},
        B={"name","default"},
        L={"name","len_name","size_type"},
        O={"name","default"},
        X={"name","value","type"},
        F={"name","type"}
    }
    
    local ret_t_pars = { -- options for return argument parts
        N={"name"},
        S={"name"},
        B={"name"},
        L={"name","len_name","bufsize"},
        O={"name","ref"},
    }        

    local lw_cmd = {}; --  //LW: commands execution

    function lw_cmd.Module(params,val,frame)
        if module_name then __err("module already defined") end        
        log(2,'Module:',params)
        local p = params:split("%s"," ","+");
        module_name = p[1]:M("^(%u%a+)$") or __err("no module name given")
        mode = M("^(%a+)$",p[1]) or "static"
        test = mode:M("static") or err("only static mode supported")
        local s = "#include <stdio.h>\n#include <lua.h>\n#include <lualib.h>\n#include <lauxlib.h>\n"
          .. f("#define LW_MODULE %s\n#define LW_MODE %s\n",module_name,mode)
        frame.add(s)
        
        
        if mode == 'static' then
            local fn = prefix .. lw_include
            local fd = io.open(fn,"r") or __err("cannot open: " .. fn)
            frame.add(fd:read("*all") or __err("cannot read: " .. fn),fn,1)
            fd:close()


            fn = prefix .. lw_code
            fd = io.open(fn,"r") or __err("cannot open: " .. fn)
            frame.add(fd:read("*all") or __err("cannot read: " .. fn),fn,1)
            fd:close()
        else -- 'extern'
            -- copy luawrap.h
            -- copy luawrap.c
            frame.add('#include "luawrap.h\n')
        end
        
        log(3,'Module:',module_name,mode)
    end

    
    function class (params,val,frame)
        if not module_name then __err("no module defined") end        
        local c = {filename=frame.filename,ln=frame.ln, methods={}}
        local p = params:split("%s"," ","+");
        
        c.name = p[1]:match("^([%a]+)$") or __err("Class must have a name [A-Z][A-Za-z0-9]+")
        c.type = p[2]:match("^([%a_]+)$") or __err("Class must have a type [A-Za-z0-9]+")
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
       if not module_name then __err("no module defined") end        
       log(5,'Alias:',params,val)
        do return end
        local a = {}
        log(3,'Alias:',a)
    end
    
    local mode = 'static'

    function lw_cmd.CallBack(params,val,frame)
        if not module_name then __err("no module defined") end        
        log(5,'CallBack:',params,val)
        
        local c = {t = 'F'}
        local p = params:split("%s"," ","+")
        
        c.name, c.type, c.mode = p[1], p[2], p[3]
        
        test = (c.name and c.type and c.mode) or _err("CallBack must have name, type and mode params");
        
        callbacks[c.name] = c
    end
    
    function lw_cmd.Accessor(params,val,frame)
        if not module_name then __err("no module defined") end        
        log(5,'Accessor:',params,val)
        do return end
    end
    
    function lw_cmd.OwnFunction(params,val,frame)
        if not module_name then __err("no module defined") end        
        log(5,'OwnFunction:',params,val)
        do return end
    end

    function lw_cmd.Finish(params,val,frame)
        if not module_name then __err("no module defined") end        
        log(5,'Finish:',params,val)
        do return end
    end
    
    function D(o) log(0,o) end


    function lw_cmd.CallBack(params,val,frame)
        if not module_name then __err("no module defined") end        
        do return end
        -- 2do: break like Class
        local cb = lw_cmd.Function(params,val,frame,{
            is_cb=true,filename=frame.filename, ln=frame.ln, self={t='O'},
            rets={}, args={}, names={}, params=params, expr=val
        })
        
        callbacks[cb.name] = cb
    end
    
    local function shift(t) return table.remove(t,1) end
    
    function lw_cmd.Function(params,val,frame)
        if not module_name then __err("no module defined") end        
        log(2,'Function:',params,val)
        local m = m or {
            filename=fname, ln=fln, rets={}, args={}, names={}, params=params, expr=val
        }
        
        local pars = params:split("%s"," ",'+');
        m.fullname = shift(pars) or __err("Function must have name parameter")
        m.fn_name = shift(pars) or __err("Function must have fn_name parameter")        
        m.proto = prototypes[m.fn_name] or __err("no prototype found for: " .. v2s(m.fn_name) )
        
        local collection
        
        if m.fullname:match("%s*(%u[%a]+)[.]([%a_]+)%s*") then
            m.cname, m.name = m.fullname:match("(%u[%a]+)[.]([%a_]+)")
            collection = (classes[m.cname] or __err("no such class: " .. v2s(m.cname))).methods
        elseif m.fullname:match("([%a_]+)") then
            m.name = m.fullname
            collection = functions
        else
            __err("Not a good name: "..v2s(m.fullname))
        end
        
        local i = 0
        local ret, args = val:match("%s*([^@]*)%s*[@]?([^@]*)%s*")
        
        test = (ret and args) or __err("malformed method:" .. v2s(m.fn_name) )
        
        D(types)
        splitList(ret,ret_t_pars,m.rets)
        splitList(args,arg_t_opts,m.args)
        
        for k,a in pairs(m.args) do
            if not a.type and a.t ~= 'K' and a.t ~= 'F'  then
                if m.proto.args[a.name] then
                    a.type = m.proto.args[a.name].type
                else
                    __err(f("In function %s untyped argument: '%s' is not in prototype",m.fullname,a.name))
                end
            end
        end
        
        collection[m.name] = m
        local s = function2C(m)
        frame.add(s)
        log(3,"Function",m)
    end
    
    local function lwcmd(lwc,frame,stack)
        local cmd,params,val = lwc:match("(%u[%a]+)%s*([^:]*)[:]?%s*(.*)%s*")
        log(1,"LWD:", lwc,cmd,params,val)
        if cmd == 'ProcFile' then
            local infile, outfile = line:match('ProcFile%s+([^%s]+)%s*([^%s]*)%s*')
            
            test =  not infile and __err("No input file in ProcFile directive")
            local new_fr = reader(infile,frame.add,frame.vadd,frame.finish)
            table.insert(stack,new_fr)
            frame = stack[#stack]
            if outfile then
                if mode == 'extern' then
                    frame.add, frame.vadd, frame.finish = output(outfile)
                else
                    __err("Cannot use ProcFile in 'static' mode");
                end
            end
            return frame 
        elseif cmd then
            if lw_cmd[cmd] then
                lw_cmd[cmd](params,val,frame)
                return frame;
            else
                __err("No such command: " .. cmd)
            end
        else
            __err(" bad cmd: " .. lwc)
        end
    end
    
    function reader(filename,add,vadd,finish)
        local f = io.open(filename, "r");
        
        if not f then
            __err("Cannot open file: '" .. filename .. "'")
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
        
        
    function process(filename,add,vadd,finish)
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
                    frame.add(line.."\n",frame.filename,frame.ln)
                end
                
                local function _err(str)
                    print("Error[" .. frame.filename ..":"
                        .. frame.ln .. "]:" .. str .."\nLine:"  
                        .. v2s(line)  )
                    os.exit(3)
                end
            
                __err = _err
                
                if ml_lwc then
                    ml_lwc = ml_lwc .. line
                    if line:match("[*][/]") then
                        frame = lwcmd(ml_lwc:gsub("\n",''):gsub("[*][/]",''),frame,stack)
                        ml_lwc = nil
                    else
                        ml_lwc = ml_lwc .. line
                    end
                elseif line:match("^%s*/[*]LW:(.*)") then -- one line LuaWrap commands
                    ml_lwc = line:match("^%s*/[*]LW:(.*)")
                elseif line:match("^%s*//LW:(.*)") then -- one line LuaWrap commands
                    lwcmd(line:match("^%s*//LW:(.*)"),frame,stack)
                elseif line:match('^[#]include%s+["]([[%a_/.]+[.]h)["]') and frame.base then -- #include
                    local incl = line:match('^[#]include%s+["]([[%a_/.]+[.]h)["]')
                    local new_fr = reader(incl,add,vadd,finish);
                    table.insert(stack,new_fr)
                    frame = stack[#stack]
                elseif line:match("^%a[%a_%s*]*[(].*;%s*$") then -- C function prototypes an typedefs
                    local proto = {
                            args={},argv={},line=line,file=frame.filename,ln=frame.ln
                        }

                    if line:match("^typedef ") then
                        log(1,"CB typedef: " .. line)
                        frame.vadd(1,"// CB typedef: " .. line)
                        line = line:gsub("^typedef%s+","")
                        line = line:gsub("[(]%s*[*]%s*([%a_]+)%s*[)]","%1")
                        proto.typedef = true
                    else
                        log(1,"C prototype: " .. line)
                        frame.vadd(1,"// C prototype: " .. line)
                    end

                    local i = 0;

                    proto.type, proto.name, proto.arglist =
                        line:match("%s*([%a_]+%s*[%s*]+)([%a_]+)%s*[(](.*)[)]")



                    for arg in (proto.arglist..','):gmatch("([^,]+),") do
                        i = i + 1
                        local t, name = arg:match("%s*([%a_]*%s*[%a_]*%s*[%a_]+[%s*]+)([%a_]+)%s*")
                        local a = {}
                        a.type, a.name = t, name;
                        a.idx = i
                        proto.args[a.name]=a
                        table.insert(proto.argv,a)
                    end
                    log(3,proto)
                    prototypes[proto.name] = proto;
                end
            end
            if frame.finish then frame.finish() end
            table.remove(stack)
            log(4,"done with file:", frame.filename)
        end
    end
    
    local function usage() 
        print("usage: " .. arg[-1] .." ".. arg[0] .. " [verbosity] <infile> [outfile]")
        os.exit(1)
    end
    
    if not arg[1] then
        usage()
    end
    
    local fname, add, vadd, finish
    
    prefix  = arg[0]:M("^(.*/)[%a_.]") or ''

    
    if arg[1]:match('^[%d]+$') then
        verbose = tonumber(arg[1])
        fname = arg[2] or usage()
        if arg[3] then
            add, vadd, finish = output(arg[3],f("%s %s %s %s %s",arg[-1],arg[0],arg[1],arg[2],arg[3]))
        end
    else
        fname = arg[1]
        if arg[2] then
            add, vadd, finish = output(arg[2],f("%s %s %s %s",arg[-1],arg[0],arg[1],arg[2]))
        end
    end
    
    process(fname, add, vadd, finish)
    
    
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
