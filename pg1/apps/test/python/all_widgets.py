#!/usr/bin/env python

#
# Show all widgets
#
# *** this is still incomplete ***
#
# This version is messy and doesn't even attempt to
# test each widget, a new version could be done using
# widget templates.

import PicoGUI
app = PicoGUI.Application('All Widgets')

tb = app.addWidget('toolbar')
sb = tb.addWidget('scrollbox')

b = tb.addWidget('button','inside')
b.text = 'simplemenu'
b = b.addWidget('button')
b.text = 'dialogbox'
b = b.addWidget('button')
b.text = 'messagedialog'

page1 = tb.addWidget('tabpage')
page1.text = 'complex'
sb = page1.addWidget('scrollbox', 'inside')

s = sb.addWidget('scroll','inside')
s.side = 'right'
s = s.addWidget('scroll')
s.side = 'bottom'
s = s.addWidget('indicator')
s.side = 'right'
s.value = 70
s = s.addWidget('indicator')
s.side = 'bottom'
s.value = 70

pbarv = s.addWidget('panelbar')
l = pbarv.addWidget('label','inside')
l.text = 'panelbar (vertical)'
l.side = 'all'
pbarv.side = 'right'
pbarh = pbarv.addWidget('panelbar')
l = pbarh.addWidget('label','inside')
l.text = 'panelbar (horizontal)'
l.side = 'all'
pbarh.side = 'bottom'

panel = pbarh.addWidget('panel')
panel.side = 'left'
panel.sizemode = 'percent'
panel.size = 50
panel.text = 'panel'

panel.addWidget('background')

page2 = page1.addWidget('tabpage')
page2.on = 1
page2.text = 'simple'
sb = page2.addWidget('scrollbox', 'inside')

f = sb.addWidget('field', 'inside')
f.text = 'field'
f.side = 'top'

textbox = f.addWidget('textbox')
textbox.text = 'textbox'
textbox.side = 'top'

btn = textbox.addWidget('button')
btn.text = 'button'
btn.side = 'top'

btn = btn.addWidget('menuitem')
btn.text = 'menuitem'
btn.side = 'top'

btn = btn.addWidget('listitem')
btn.text = 'listitem'
btn.side = 'top'

btn = btn.addWidget('flatbutton')
btn.text = 'flatbutton'
btn.side = 'top'

btn = btn.addWidget('checkbox')
btn.text = 'checkbox'
btn.side = 'top'

btn = btn.addWidget('radiobutton')
btn.text = 'radiobutton 1'
btn.side = 'top'
btn = btn.addWidget('radiobutton')
btn.text = 'radiobutton 2'
btn.side = 'top'
btn = btn.addWidget('radiobutton')
btn.text = 'radiobutton 3'
btn.side = 'top'

l = btn.addWidget('label')
l.text = 'Normal label'
l = l.addWidget('label')
l.text = 'Opaque label'
l.transparent = 0

term = l.addWidget('terminal')
term.side = 'top'
term.size = 100

# Write to the terminal after everything's sized,
# since writing to a 0-size terminal is no fun
app.server.update()
term.write('terminal\n\rwidget')

def eventwatch(ev):
    print repr(ev),
    try:
        print ev.widget
    except:
        print
app.link(eventwatch)

app.run()
