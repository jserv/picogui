#!/usr/bin/env python

import PicoGUI
app = PicoGUI.Application('infilter')
print 'filter handle is', app.server.mkinfilter(0, 0xFFFFFFFFL, 0)
def myfilter(ev, sender):
    if ev.name == 'idle': return
    if hasattr(ev, 'sender'):
        print (ev.dev, ev.name, ev.sender, ev.__dict__)
    else:
        print 'HEY! %s is not a trigger' % repr(ev)
app.link(myfilter)
app.addWidget('Button')
app.run()
