# Example of a widget template for picosm
import PicoGUI

app = PicoGUI.Application('PicoGUI Login', PicoGUI.WTFile())
app.name = "PSMApp"

# A spiffy icon from KDE's Crystal icon set
icon = app.addWidget('label')
icon.side = 'left'
icon.image = open('kscreensaver.png').read()

title = icon.addWidget('label')
title.font = ':20:bold'
title.text = 'Welcome to PicoGUI'

subtitle = title.addWidget('textbox')
subtitle.text = 'To begin your PicoGUI session, enter your username and password below. You may also sit here and stare at the pretty icon to the left of this message, or admire the fact that all these extra useless widgets were brought to you in a conveniently packaged Widget Template (tm) for your enjoyment.'
subtitle.readonly = 1

loginbox = subtitle.addWidget('box')
loginbox.name = 'PSMLoginBox'

passwdbox = loginbox.addWidget('box')
passwdbox.name = 'PSMPasswdBox'

buttonbox = passwdbox.addWidget('toolbar')
buttonbox.side = 'bottom'
buttonbox.name = 'PSMButtonBox'

open("picosm.wt", 'w').write(app.server.dump())
