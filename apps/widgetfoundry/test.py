#!/usr/bin/env python
import PicoGUI
import WidgetProperties
import XWT

app = PicoGUI.Application('Widget Foundry')

w = app.addWidget('label')
w.properties = {}

toolbox = app.addWidget('box')
toolbox.side = 'left'
toolbox.margin = 0
toolbox.transparent = 1
toolbox_bar = toolbox.addWidget('panelbar','inside')
toolbox_bar.side = 'right'
toolbox_bar.bind = toolbox
toolbox_title = toolbox_bar.addWidget('label','inside')
toolbox_title.text = 'Tools'
toolbox_title.direction = 90
toolbox_title.side = 'all'

title = toolbox_bar.addWidget('label','after')
title.text = 'Widget Properties:'
title.align = 'left'

w.text = 'Hello World, this is some long text and stuff... yay'
w.font = ':20'
w.side = 'all'

wpbox = title.addWidget('scrollbox')
wpbox.side = 'all'
WidgetProperties.PropertyList(app,w,wpbox)

b = title.addWidget('button')
b.side = 'bottom'
b.text = 'Dump XML'
def dump(ev, button):
    print XWT.dumpWidget(w,app)
app.link(dump,b,'activate')

foo = w.addWidget('label','inside')
foo.side = 'left'
foo.text = 'Yay'

foo = w.addWidget('label','inside')

print XWT.dumpWidget(w,app)
app.run()
