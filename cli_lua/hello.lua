-- hello.lua

dofile("pico.lua")

cnt =0
function cb_hello()
    cnt = cnt + 1
    Pico:replaceText(label,"Hello "..cnt)
    print("Hello "..cnt)
    Pico:update()
end

win = Pico:Register("test",Pico.app_normal)

--t={from=win, type=Pico.t_keyup, key=Pico.key_0, mods=Pico.mod_lshift,
--    flags=Pico.kf_focused, consume=0}
--print(Pico:inFilterSend(t))

label = Pico:Widget{
	Pico.w_label; 
	 text = "Hello", 
	 side = Pico.s_all, 
	 font = {nil,Pico.fstyle_bold,24},
	 triggermask = Pico.t_mouses,
	 callback = {Pico.we_activate, cb_hello}
	-- bgcolor=Pico.c_green
	 }

Pico:run()