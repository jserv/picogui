#!./sol -f
-- conv.lua
-- convert #define in headers to Pico const variables
-- 

execute("cat constants.h network.h pgkeys.h canvas.h | grep '#define' > const.tmp")

f = readfrom("const.tmp")
fout = writeto("const.lua")

write(fout, "-- Generated from constants.h network.h pgkeys.h canvas.h\n")
write(fout, "-- Do not manually update\n")
write(fout, "-- Shorten pg_widget to pg_w, pg_trigger to pg_t\n")
write(fout, "-- convert eg pgc_red to pico.usualcolor.red \n")
write(fout, strrep("-",60).."\n")
write(fout, "Pico = {} \n")
write(fout, "Pico.handler={}  -- event handler \n")
write(fout, "Pico.usualcolor = {}")
write(fout, "\n")

while 1 do 
    ll = read(f, "*l")
    if not ll then break end
    
    _,_, name, value = strfind(ll, "^#define%s+([%w_]+)%s+(%S+).*")
    print(name, value)
    if name and value then
	-- name conversion
	name = strlower(name)
	name = gsub(name, "^pg_widget", "Pico.w")
	name = gsub(name, "^pg_trigger", "Pico.t")
	name = gsub(name, "^pgreq", "Pico.req")
	name = gsub(name, "^pgkey", "Pico.key")
	name = gsub(name, "^pgmod", "Pico.mod")
	name = gsub(name, "^pgth",  "Pico.th")
	name = gsub(name, "^pgc_",   "Pico.usualcolor.")
	name = gsub(name, "^pgres", "Pico.res")
	name = gsub(name, "^pg_",   "Pico.")
	name = gsub(name, "^pgdm", "Pico.dm")
	name = gsub(name, "^pgcanvas", "Pico.canvas")
	
	-- value conversion
	_, _, v = strfind(value, "0[xX](%w+)") -- eg 0xFFFF
	if v then
	    vout = tonumber(v, 16)
	else
	    _,_, v = strfind(value,"%(1%<%<(%d+)%)")  -- eg (1<<20)
	    if v then
		vout = lshift(1, v)   
	    else
		vout = tonumber(value)  -- pure number, or nil 
	    end
	end
	
	if vout then write(fout, format("%-35s = %d\n", name, vout)) end
    end
end

readfrom()
writeto()

print("Checking duplication...")
execute("cut -f1 -d ' ' const.lua | sort | uniq -D")
print("Done")