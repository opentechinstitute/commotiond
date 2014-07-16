#!/bin/sh

KW='rawget|setmetatable|io|arg|table|pairs|error|string|tostring|os|type|rawset|tonumber|print|ipairs'

(${LUA}c -p -l luaqwrap.lua | grep '_ENV' | sed 's/.*\[\([0-9]*\)\].*"\(.*\)"/luaqwrap.lua:\1 : Using Global: "\2"/;' | egrep -v ${KW}) | sort -n -k 2 -t ':'



