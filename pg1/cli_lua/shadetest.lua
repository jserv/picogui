-- shadetest.lua
-- demo stardard shades
dofile("pico.lua")

-- callbacks
function cb_quote()
    Pico:Shademsg( "Quote"..
		  "\nAndrew, I don't care if your\n"..
		  "sister put an ice cube down\n"..
		  "your underware she's a girl\n"..
		  "and girls will do that.\n"..
		  "That doesn't mean that you\n"..
		  "can hit her with the dog.\"\n\n".. 
		  "-Lt. Col Henry Blake,\n  MASH 4077"
	    )	
end

function cb_askyes()
    print ("yes")
    Pico:Shademsg ("Boom!"..
		       "\nYou have only "..date("%H")..
		       "\nhours until critical mass"
		  )
    Pico:closeShade(nil, ask_close)
    ask_close = nil
end

function cb_askno()
    print ("no")
    Pico:closeShade(nil, ask_close)
    ask_close = nil
end

ask_close = nil
function cb_ask()
    if not ask_close then
	ask_close = Pico:Shadeask(
		  "Really???\n"..
		  "You just clicked a button.\n"..
		  "Are you sure you want\n"..
		  "to initiate nuclear fusion?"
		  , cb_askyes, cb_askno )
    end
end

function cb_inputok()
    local str = Pico:getShadeinput(id_input)
    Pico:Shademsg ("You entered "..str)
    Pico:closeShade(nil, input_close)
    id_input = nil
end

function cb_inputcancel()
    Pico:closeShade(nil, input_close)
    id_input = nil
end

id_input = nil
input_close = nil
function cb_input()
    if not id_input then
	input_close, id_input = Pico:Shadeinput (
	    "What is the air-speed velocity of an unladen swallow?", "100"
	    , cb_inputok, cb_inputcancel )
	print (input_close, id_input)
    end
end

id_fileopen = nil
fileopen_close = nil

function cb_fopen()
    Pico:closeShade(nil, fileopen_close)
    id_fileopen = nil
end

function cb_fileopen()
    if not id_fileopen then
	fileopen_close, id_fileopen = Pico:Shadefile (cb_fopen, cb_fopen, "/home/fanhow/my/", "pico.lua")-- default open
    end
end
-- starts

Pico:Register("Standard Shades") 

box = Pico:Widget { Pico.w_scrollbox }
Pico:Group(box)
Pico:bind(Pico.we_activate
     , Pico:Widget{ Pico.w_button; text = "Shademsg: Quote", side = Pico.s_top }
     , cb_quote )
     
Pico:bind(Pico.we_activate
     , Pico:Widget{ Pico.w_button; text = "Shadeask: boom", side = Pico.s_top }
     , cb_ask )    
     
Pico:bind(Pico.we_activate
     , Pico:Widget{ Pico.w_button; text = "Shadeinput: Enter a string", side = Pico.s_top }
     , cb_input )  

Pico:bind(Pico.we_activate
     , Pico:Widget{ Pico.w_button; text = "Shadefile: Open file", side = Pico.s_top }
     , cb_fileopen )      
     
Pico:run()


