#!/usr/bin/env python
import PicoGUI

app = PicoGUI.Application("Canvas test")
canvas = PicoGUI.Canvas(app.addWidget('canvas'))

canvas.setcolor(0xFF0000)
canvas.rect(10,10,50,50)

canvas.setcolor(0x0000FF)
canvas.rect(30,30,80,80)

app.run()
