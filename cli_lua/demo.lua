-- demo.lua
dofile("pico.lua")

cnt = 0
function cb_hello ()
    cnt = cnt + 1
    Pico:replaceText(label, "Hello World "..cnt)
    Pico:setProperty(indicator, {value = cnt})
--    Pico:Dialog{ "test text"}
end

win = Pico:Register ("demo", Pico.app_normal)
assert(win, "Error starting") 

toolbar = Pico:Widget{Pico.w_toolbar}
box = Pico:Widget{Pico.w_box; size = 100 }
indicator = Pico:Widget {Pico.w_indicator; side = Pico.s_top}

Pico:Widget {Pico.w_label;
	     side = Pico.s_top,
	      align = Pico.a_left,
	      text = "100 clicks"
	    }
Pico:Widget {Pico.w_label;
	     transparent = 0,
	      text = "Click the buttons above",
	      font = {"Courier",nil,nil}
	    }
Pico:Widget {Pico.w_label;
	     side = Pico.s_bottom,
	      align = Pico.a_left,
	      text = "0 clicks"
	    }
	
label = Pico:Widget{
	    Pico.w_label;
	      text = "Foo...",
	      font = {"Utopia",nil,nil},
	      side = Pico.s_all,
	      transparent = 0
	    }
	    
-- group following inside box	    
Pico:Group(box) 
text = Pico:Widget{Pico.w_label;
	 side = Pico.s_all,
	  text = " test line \n another line "
	}
    
Pico:Widget{Pico.w_scroll, box, Pico.derive_before; bind = box }

-- group following widgets inside toolbar
Pico:Group(toolbar)  
Pico:Widget{Pico.w_button; text = "Button"}
Pico:Widget{Pico.w_button; text = "Toggle", 
	extdevents = Pico.exev_toggle}
Pico:Widget{Pico.w_checkbox; text = "Checkski!", side = Pico.s_left}
Pico:Widget{Pico.w_flatbutton; text = "Flatbutton"}
Pico:Widget{
	    Pico.w_button; 
	    text = "Bitmap", 
	    bitmap="data/tux.pnm",
	    bitmask="data/tux_mask.pnm"
	   }
Pico:Widget{
	Pico.w_button;
	 side = Pico.s_right, 
	 text = "Hello world!", 
	 callback = {Pico.we_activate, cb_hello}
	}


Pico:run()	

	      
	    
    