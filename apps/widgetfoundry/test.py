#!/usr/bin/env python
import PicoGUI
import WidgetProperties

app = PicoGUI.Application('Widget Foundry')

w = app.addWidget('label')

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

wpbox = title.addWidget('scrollbox')
wpbox.side = 'all'

WidgetProperties.PropertyList(app,w,wpbox)

app.run()
