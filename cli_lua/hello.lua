-- hello.lua

dofile("pico.lua")

cnt =0
close_ask = nil -- used to shutShade

function cb_hello()
    cnt = cnt + 1
    Pico:replaceText(label,"Hello "..cnt)
    print("Hello "..cnt)
    if not close_ask then
	close_ask = Pico:Shademsg(" to be or not to be? ") --, cb_yes, cb_no)
    end
end

function cb_yes()
    print ("yes")
    Pico:closeShade(nil, close_ask)
    close_ask = nil
end

function cb_no()
    print ("no")
    Pico:closeShade(nil, close_ask)
    close_ask = nil
end
    
win = Pico:Register("test")

--t={from=win, type=Pico.t_keyup, key=Pico.key_0, mods=Pico.mod_lshift,
--    flags=Pico.kf_focused, consume=0}
--print(Pico:inFilterSend(t))

label = Pico:Widget{
	Pico.w_label; 
	 text = "Hello", 
	 side = Pico.s_all, 
	 font = {nil,Pico.fstyle_bold,24},
	 triggermask = Pico.t_mouses
--	 callback = {Pico.we_activate, cb_hello}
	-- bgcolor=Pico.c_green
	 }
Pico:bind (Pico.we_activate, label, cb_hello)
--Pico:Dialog {" test dialog\n another line" }

Pico:run()