#!/usr/bin/env python

import PicoGUI
app = PicoGUI.Application('Pseudo-grid-test')
box = app.addWidget('scrollbox')
for y in range(0,20):
    line = box.addWidget('box','inside')
    line.side = 'top'
    line.transparent = 1
    for x in range(0,20):
        w = line.addWidget('button','inside')
        w.text = 'grid'
app.run()
