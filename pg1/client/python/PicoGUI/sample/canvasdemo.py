#!/usr/bin/env python
import PicoGUI

app = PicoGUI.Application("Canvas test")
canvas = app.addWidget('canvas')

canvas.grop.setcolor(0xFF0000)
canvas.grop.rect(10,10,50,50)

canvas.grop.setcolor(0x0000FF)
canvas.grop.rect(30,30,80,80)

canvas.grop.setcolor(0xFF9900)
canvas.grop.arc(30,30,80,80,90,270)

app.run()
