#!/usr/bin/python2.2
# Test for PicoGUI Python client library. If Python batch requests
# (via hold()/flush()) are working, this C version shouldn't be
# much faster than scrollbar_fun.py

import PicoGUI, math
app = PicoGUI.Application('Scrollbar Fun - Python')

widgets = []
for i in range(0,40):
    widgets.append(app.addWidget('scroll'))
    widgets[i].number = i
    widgets[i].side = 'left'

t = 0
while 1:
    app.server.hold()
    t += 0.04
    for w in widgets:
        w.value = 50 +50 * math.sin( (w.number*0.2) - t )
    app.server.update()
    app.server.flush()
