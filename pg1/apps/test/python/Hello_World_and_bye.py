#!/usr/bin/env python

# Like Hello_World1 but with a close button
import PicoGUI
app = PicoGUI.Application('Greetings')
l = app.addWidget('Label')
l.side = 'All'
l.text = 'Hello, World!'
l.font = ':24:Bold'
bye = app.addWidget('Button')
bye.text = 'Yeah, all right, now go away'
bye.side = 'Bottom'
def closeapp(ev, button):
    app.send(app, 'stop')
app.link(closeapp, bye, "activate")
app.run()
