#!/usr/bin/env python

# As simple as it gets
import PicoGUI
# for PG_APP_TOOLBAR you'd use PicoGUI.ToolbarApp()
app = PicoGUI.Application('Tab test')
p = app.addWidget('Tabpage')
p.text = '1'
p.alt_size = 0

btn = p.addWidget('checkbox', 'inside')
btn.text = 'hide'
btn.side = 'top'

def show_hide(ev):
    print 'resizing to', p.alt_size
    s = p.size
    p.size = p.alt_size
    p.alt_size = s
app.link(show_hide, btn, 'activate')

def change_side(ev):
    p.side = ev.widget.text
btn = btn.addWidget('radiobutton')
btn.text = 'top'
btn.side = 'top'
btn.on = 1
app.link(change_side, btn, 'activate')
btn = btn.addWidget('radiobutton')
btn.text = 'left'
btn.side = 'top'
app.link(change_side, btn, 'activate')
btn = btn.addWidget('radiobutton')
btn.text = 'bottom'
btn.side = 'top'
app.link(change_side, btn, 'activate')
btn = btn.addWidget('radiobutton')
btn.text = 'right'
btn.side = 'top'
app.link(change_side, btn, 'activate')

n = p.addWidget('Tabpage')
n.text = 'special'
n.addWidget('Label', 'inside').text = 'This tabpage is different from the others.'
t = PicoGUI.Widget(app.server, n.tab, n)
t.side = 'right'
for i in range(2, 6):
    n = n.addWidget('Tabpage')
    n.text = str(i)
    n.addWidget('Label', 'inside').text = str(i)
app.run()
