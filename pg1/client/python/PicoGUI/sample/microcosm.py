#!/usr/bin/env python

# Demonstrate that everything in picogui is a widget :)
import PicoGUI

app = PicoGUI.Application('Microcosm')

p1 = app.addWidget('panel')
p1.side = 'left'
p1.sizemode = 'percent'
p1.size = 70
p1.text = 'Panel widget'
p1.addWidget('background','after')

p2 = p1.addWidget('panel','inside')
p2.side = 'top'
p2.sizemode = 'percent'
p2.size = 40
p2.text = 'Another panel widget'
p2.addWidget('background','after')

l = p2.addWidget('label','inside')
l.font = ':20'
l.text = 'Hello'
l.side = 'all'

app.run()
