-- pico.lua 
--
debug = print

dofile("const.lua")

-- wrapper methods

function Pico:Dialog(table)
    -- FIXME: doesnot work yet.
    -- a modal dialog with button
    -- message in table postion 1, optinal title, type, icon, btn1, btn2
    -- return 1 for when click btn1, or nil when click btn2 or close
    
    if not table.title then table.title = "Dialog" end 
    if not table.type then table.type = "message" end
    if not table.icon then table.icon = table.type end
    
    if table.type=="question" then
	-- question box
	if not btn1 then btn1 = "YES" end
	if not btn2 then btn2 = "NO" end
    elseif table.type=="input" then
	-- input box
	if not btn1 then btn1 = "CONFIRM" end
	if not btn2 then btn2 = "CANCEL" end
    else 
	-- message
	if not btn1 then btn1 = "OK" end
    end
	
--    Pico:netcall { Pico.req_mkcontext }  -- a new context for us

    -- create widgets
    Pico:Group( Pico:Widget{ Pico.w_popup;
		    absolutex = Pico.popup_center, 
		    absolutey = -1, 
		    width = 100, 
		    height = 100
		    } 
		)  --create popup and put following inside
    Pico:Widget{ Pico.w_label;
		    text = title,
		    transparent = 0,
		    thobj = Pico.th_o_label_dlgtitle,
		    side = Pico.s_top
		    }
    Pico:Widget{ Pico.w_label;
		 text = table[1],
		 side = Pico.s_all
		}
		
    Pico:Group( Pico:Widget{ Pico.w_toolbar; size = Pico.s_bottom } )
    Pico:netcall{ Pico.req_setpayload, 
	Pico:Widget { Pico.w_button; text = table.btn1, 
		align = Pico.a_right }, 1 }
    if table.btn2 then 
	Pico:netcall { Pico.req_setpayload,
	    Pico:widget { Pico.w_button; text = table.btn2, 
		align = Pico.a_right }, 0 }
    end
    
    Pico:update() 
    local ret
--~     
--~     print("Before loop")
--~     while 1 do	
--~ 	Pico:request(Pico.req_wait)  -- put on server wait queue
--~ 	restype, event, from, param = Pico:response(-1)
--~ 	print (restype, event, from, param )
--~ 	if restype==Pico.response_event then
--~ 	    ret = Pico:netcall { Pico.req_getpayload, from } 
--~ 	    if ret~=nil then break end
--~ 	end
--~     end    
--~     print("After loop")
--~     Pico:netcall { Pico.req_rmcontext }     
--~     return ret
end

		
function Pico:Register(name, apptype, table, host, display)
    -- register app window with given name and type
    if not Pico:connect(host, display) then return end
    
    local hName = Pico:String(name)
    if not hName then return nil end
    local app = Pico:netcall { Pico.req_register, hName, apptype }
    if app then Pico:Group(app) end
    return app
end

function Pico:setProperty(widget, table)
    -- widget in table[1]
    local k,v,const, suf
    if not table or not widget then return end
    
    for k,v in table do
	pair = Pico.wplookup[k]  -- get const with suffix
	if not pair then 
	    -- special? (not in Pico.wp)
	    if tonumber(k) then 
		-- positional arg, do nothing
	    elseif k == "callback" then
		Pico:addHandler(v[1], v[2], widget) 
	    else
		Pico:errmsg(widget.." has no property "..k )
	    end
	else
	    _, _, const, suf = strfind(pair, "(%d+)(%a*)")
	    
	    if suf == "S" then -- string, create and use handle
		v = Pico:String(v)
	    elseif suf == "F" then -- Font, create and use handle
		v = Pico:Font(v[1],v[2],v[3])
	    elseif suf == "C" then -- Color in RGBA format
	    elseif suf == "B" then -- Bitmap filename
		v = Pico:BitmapFile(v)
	    end
	    debug(k,v,const,suf)
	    const=tonumber(const)
	    if not Pico:netcall{ Pico.req_set, widget, v, const } then
		Pico:errmsg(widget.." could not set property '"..k.."' to value "..v)
	    end
	end
    end
end

function Pico:String(str)
    return Pico:netcall{ Pico.req_mkstring, Pico:raw2str(str) }
end

function Pico:Font(name, style, size)
    -- make a new font, return handle
    if not name then name = "" end
    if not style then style = Pico.fstyle_default end
    if not size then size = 0 end 
    local nm = Pico:raw2str(name)
    local pad = strrep("0", 80 - strlen(nm))
    return Pico:netcall{ Pico.req_mkfont, pad..nm, style, size }
end

function Pico:BitmapFile(filename)
    -- load bitmap file to server, return handle
    local f = readfrom(filename)
    if not f then return end
    local str = read(f,"*a")
    closefile(f)
    return Pico:netcall{ Pico.req_mkbitmap, Pico:raw2str(str) }
end

function Pico:Widget(table) 
    -- create a widget with properties in table
    local w = Pico:makeWidget(table[1],table[2],table[3]) -- widget type, buddy, rship respectively
    if w then Pico:setProperty(w, table) end
    return w
end

function Pico:Group(widget) 
    -- change lastWidget and rship to group following widgets inside
    Pico.lastWidget = widget
    Pico.defaultRship = Pico.derive_inside
end

function Pico:update()
    Pico:netcall{ Pico.req_update }
end

function Pico:replaceText(widget, string)
    -- replace text property with 'string'
    if type(widget)~="number" or type(string)~="string" then
	return nil end
    local old = Pico:netcall{ Pico.req_get, widget, Pico.wp_text }
    Pico:setProperty(widget, { text = string })
    if old then Pico:netcall{ Pico.req_free, old } end
end

function Pico:makeWidget(widtype, buddy, rship)
    -- create and attach a widget to a buddy widget
    if not buddy then buddy=Pico.lastWidget end
    if not rship then rship=Pico.defaultRship end
    local t = Pico:netcall{ Pico.req_mkwidget, rship, widtype, buddy }
    if t then
	Pico.lastWidget = t
	Pico.defaultRship = Pico.derive_after
    end
    return t
end

function Pico:addHandler(event, func, widget)
    -- bind a callback function to event and optional widget
    -- no chain, it's not easy to use, overwrite func instead
    if not func then  -- nil func remove the binding
	if not widget then -- nil widget remove all 
	    Pico.handler[event] = nil 
	else -- remove only that widget
	    Pico.handler[event][widget] = nil
	end
	debug ("hander removed for event: "..event.." and widget "..(widget or "all"))
	return 
    end
    if type(func)~="function" then 
	Pico:errmsg("Handler is not a function. It's type: "..type(t))
	return 
    end
    
    local t = Pico.handler[event]
    if t and widget then -- append a widget
	    t[widget] = 1
	    t["all"] = nil
    else -- nil widget = all
	Pico.handler[event] = {}  -- clear
	t = Pico.handler[event]
	t["all"] = 1
    end
    t["func"] = func
    debug("handler added for event: "..event.." and widget "..(widget or "all"))
    
end

function Pico:run()
    -- event loop
    local restype, event, from, param, quit, t

    -- add default handler
    Pico:addHandler(Pico.we_close, function () return 1 end) -- quit
    
    while 1 do
        Pico:update()
	Pico:request(Pico.req_wait)  -- put on server wait queue
	restype, event, from, param = Pico:response(-1)
	if restype==Pico.response_event then
	    -- call event handler(s)
	    t = Pico.handler[event]
	    if t and (t["all"] or t[from]) then 
		debug ("handler called for event "..event.." and widget "..(t["all"] or t[from]))
		quit = t["func"](event, from, param) 
	    end
	    if event==Pico.we_close and quit then Pico:close() break end
	end
    end    
end


-- Pico netcore protocol
Pico.proto = {}
Pico.proto[Pico.req_appmsg] = "%08x"
Pico.proto[Pico.req_attachwidget]  = "%08x%08x%04x0000"
Pico.proto[Pico.req_chcontext] = "%08x%04x0000"
Pico.proto[Pico.req_checkevent] = ""
Pico.proto[Pico.req_createwidget] = "%04x0000"
Pico.proto[Pico.req_drivermsg] = "%08x%08x"
Pico.proto[Pico.req_dup] = "%08x"
Pico.proto[Pico.req_findthobj] = "%s"
Pico.proto[Pico.req_findwidget] = "%s"
Pico.proto[Pico.req_focus] = "%08x"
Pico.proto[Pico.req_free]  = "%08x"
Pico.proto[Pico.req_get]   = "%08x%04x0000"
Pico.proto[Pico.req_getcontext] = ""
Pico.proto[Pico.req_getfstyle] = "%04x0000"
Pico.proto[Pico.req_getinactive] = ""
Pico.proto[Pico.req_getmode] = ""
Pico.proto[Pico.req_getpayload] = "%08x"
Pico.proto[Pico.req_getresource] = "%08x"
Pico.proto[Pico.req_getstring] = "%08x"
Pico.proto[Pico.req_infiltersend] = "%s"  -- use wrapper instead
Pico.proto[Pico.req_loaddriver] = "%s"
Pico.proto[Pico.req_mkarray] = "%08x"
Pico.proto[Pico.req_mkbitmap] = "%s"
Pico.proto[Pico.req_mkcontext] = ""
Pico.proto[Pico.req_mkcursor] = ""
Pico.proto[Pico.req_mkfont] = "%s%08x%04x0000"
Pico.proto[Pico.req_mkinfilter]= "%08x%08x%08x"
Pico.proto[Pico.req_mkstring] = "%s"
Pico.proto[Pico.req_mktheme] = "%s"
Pico.proto[Pico.req_mkwidget] = "%04x%04x%08x"
Pico.proto[Pico.req_newbitmap] = "%04x%04x"
Pico.proto[Pico.req_ping]   = ""
Pico.proto[Pico.req_register] = "%08x%04x0000"
Pico.proto[Pico.req_regowner] = "%04x"
Pico.proto[Pico.req_render] = "%08x%08x"
Pico.proto[Pico.req_rmcontext] = ""
Pico.proto[Pico.req_set] = "%08x%08x%04x0000"
Pico.proto[Pico.req_setcontext] = "%08x"
Pico.proto[Pico.req_setinactive] = "%08x"
Pico.proto[Pico.req_setmode] = "%04x%04x%04x%04x%08x"
Pico.proto[Pico.req_setpayload]= "%08x%08x"
Pico.proto[Pico.req_sizebitmap ] = "%08x"
Pico.proto[Pico.req_sizetext] = "%08x%08x"
Pico.proto[Pico.req_thlookup] = "%04x%04x%"
Pico.proto[Pico.req_traversewgt ] = "%08x%04x%04x"
Pico.proto[Pico.req_unregowner ] = "%04x"
Pico.proto[Pico.req_update] = ""
Pico.proto[Pico.req_updatepart] = ""
Pico.proto[Pico.req_wait] = ""
Pico.proto[Pico.req_writeto] = "%08x"

function Pico:netcall(req)
    -- request and param in the req table in position, max 9
    local spec = Pico.proto[ req[1] ]  -- request type in position 1
    if spec==nil then 
	Pico:errmsg("unknown net protocol: "..req[1])
	return
    end
    local s
    if spec=="" then
	s=""
    else
	s = format(spec,req[2],req[3],req[4],req[5],req[6],req[7],req[8],req[9])
    end
    Pico:request(req[1], s)

    return Pico:response()
end

Pico.wplookup = {
-- widget properties 
-- quick lookup table
-- N: Numeric
-- S: String
-- F: Font (name,style,size)
-- C: Color RRGGBB
-- B: Bitmap filename
    size="1N",
    side="2N",
    align="3N",
    bgcolor="4C",
    color="5C",
    sizemode="6N",
    text="7S",
    font="8F",
    transparent="9N",
    bordercolor="10N",
    bitmap="12B",
    lgop="13N",
    value="14N",
    bitmask="15B",
    bind="16H",
    scroll_x="17N",
    scroll_y="18N",
    scroll="18N",
    hotkey="19N",
    extdevents="20N",
    direction="21N",
    absolutex="22N",
    absolutey="23N",
    on="24N",
    thobj="25N",
    name="26",
    publicbox="27N",
    disabled="28N",
    margin="29N",
    textformat="30N",
    triggermask="31N",
    hilighted="32N",
    selected="33N",
    selected_handle="34N",
    autoscroll="35N",
    lines="36N",
    preferred_w="37N",
    preferred_h="38N",
    panelbar="39N",
    auto_orientation="40N",
    thobj_button="41N",
    thobj_button_hilight="42N",
    thobj_button_on="43N",
    thobj_button_on_nohilight="44N",
    panelbar_label="45N",
    panelbar_close="46N",
    panelbar_rotate="47N",
    panelbar_zoom="48N",
    bitmapside="49N",
    password="50N",
    hotkey_flags="51N",
    hotkey_consume="52N",
    width="53N",
    height="54N",
    spacing="55N",
    minimum="56N",
    multiline="57N",
    selection="58N",
    readonly="59N",
    insertmode="60N",
    type="61N"
}
-- composite
Pico.t_mouses = Pico.t_pntr_relative + Pico.t_up + Pico.t_down + Pico.t_move + Pico.t_drag + Pico.t_pntr_status + Pico.t_scrollwheel + Pico.t_release + Pico.t_touchscreen + Pico.t_ts_calibrate

Pico.t_keys = Pico.t_keyup + Pico.t_keydown + Pico.t_char + Pico.t_key_start + Pico.t_key

-- internal functions --

function Pico:errmsg(msg)
    -- generic error handling, can overide to suit your application
    print("Pico_lua ERROR: "..msg)
end

-- format data
-- HACK, in pure Lua, no way to pad raw data to eg, u32, unless via string
function Pico:raw2str(data)
    -- convert each byte of data into its string representation in hex, eg 1 -> "01"
    -- HACK, have to format in string then convert back to raw data.
    return gsub(data,"(.)", function (c) return format("%02x",strbyte(c)) end )
end

function Pico:str2raw(str)
    -- convert string to its raw format
    return gsub (str,"(..)",function (v) return strchar(tonumber(v,16)) end )
end

function Pico:u16 (str,pos)
    -- convert string to u16
    return strbyte(str,pos)*256 + strbyte(str,pos+1)
end

function Pico:u32 (str, pos)
    -- convert string to u32
    return ((strbyte(str,pos)*256 + strbyte(str,pos+1))*256 + strbyte(str,pos+2))*256 + strbyte(str,pos+3)
end

-- network layer ---

function Pico:request(req, data) 
    debug("request: "..req)
    local size=0, str

    if not Pico.con then Pico:errmsg("No connection") return end
    
    data = data or ""
    Pico.id = req 
    size=strlen(data) / 2
    
    -- header
    str=format("%08x%08x%04x0000%s", Pico.id, size, req, data) 
    debug(str) 

    str = Pico:str2raw(str)
    Pico.con:send(str)
end

function Pico:response(timeout)
    local s, restype,  id

    if not Pico.con then Pico:errmsg("No connection") return end

    if not timeout then
	Pico.con:timeout(1)
    else
	Pico.con:timeout(timeout) -- negative block forever
    end
    
    local s, err = Pico.con:receive(12)

    if err then Pico:errmsg(err) return end

    if not s or strlen(s) < 12 then Pico:errmsg("No response") return end
    debug("response: "..gsub(s,"(.)", function (c) return format("%02x",strbyte(c)) end ))
    
    restype = Pico:u16(s,1)
    
    if restype == Pico.response_err then 
	local errt, msglen
	errt = Pico:u16(s,3)
	msglen = Pico:u16(s,5)	
	id = Pico:u32(s,9)
	if Pico.id == nil or Pico.id ~= id then 
	    Pico:errmsg("Pico.resp.err, Id: "..id.." not match with the requested: "..Pico.id) 
	    return
	end
	if msglen > 0 then 
	    msg = Pico.con:receive(msglen)
	    Pico:errmsg("Pico.resp.err, msg="..msg.." errt="..errt)
	    return
	end
    elseif restype == Pico.response_ret then
	id = Pico:u32(s,5)
	if Pico.id == nil or Pico.id ~= id then 
	    Pico:errmsg("Pico.resp.ret, Id: "..id.." not match with the requested: "..Pico.id)
	    return
	end
	
	local data = Pico:u32(s,9)
	return data
	
    elseif restype == Pico.response_event then
	local event, from, param
	event = Pico:u16(s,3)
	from = Pico:u32(s,5)
	param = Pico:u32(s,9)
	return Pico.response_event, event, from, param
	
    elseif restype == Pico.response_data then
	local size,len
	id = Pico:u32(s,5)
	if Pico.id == nil or Pico.id ~= id then 
	    Pico.errmsg("Pico.resp.data, Id: "..id.." not match with the requested: "..Pico.id)
	    return
	end
	
	size = Pico:u32(s,9)
	
	if size > 0 then
	    data,err = Pico.con:receive(size)
	    if err then debug(err) end
	    debug("resp size: "..size.." data: "..gsub(data,"(.)", function (c) return format("%02x",strbyte(c)) end))
	    return data
	end 	    
    else
	Pico.errmsg("unknown response type: "..restype)
    end
    
end
    
function Pico:connect(host, display)
    local s,err
    
    Pico.host = host or "localhost"
    Pico.display = Pico.request_port or Pico.request_port + display 
 
    Pico.con, err = connect(Pico.host, Pico.display)
    if not Pico.con then 
	Pico.errmsg("Failed to connect to server "..Pico.host..":"..Pico.display.." . Error is: "..err) return end
	
    -- get hello 
    s=Pico.con:receive(8)
 --   if strsub(s,1,4)~=Pico.magic then 
    if Pico:u32(s,1) ~= Pico.request_magic then
	Pico.errmsg( "Bad Magic"..s) return end
    if Pico:u16(s,5) ~= Pico.protocol_ver then
	Pico.errmsg( "Bad proto version: "..Pico:u16(s,5) ) return end
    return 1 -- ok
end

function Pico:close()
    if Pico.con then Pico.con:close() end
end

