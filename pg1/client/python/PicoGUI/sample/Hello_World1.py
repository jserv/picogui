#!/usr/bin/env python

# As simple as it gets
import PicoGUI
# for PG_APP_TOOLBAR you'd use PicoGUI.ToolbarApp()
app = PicoGUI.Application('Greetings')
l = app.addWidget('Label')
l.side = 'All'
l.text = 'Hello, World!'
l.font = ':24:Bold'
app.run()
