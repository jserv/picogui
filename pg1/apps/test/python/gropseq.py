#!/usr/bin/env python
import PicoGUI

seq = PicoGUI.Gropseq()

seq.setcolor(0xFF0000)
seq.rect(10,10,50,50)

seq.setcolor(0x0000FF)
seq.rect(30,30,80,80)

seq.setcolor(0xFF9900)
seq.arc(30,30,80,80,90,270)

app = PicoGUI.Application("Canvas test")
canvas = app.addWidget('canvas')

canvas.grop.setcolor(0xFFFFFF)
canvas.grop.rect(0,0,120,120)

canvas.gropseq(seq)

canvas.grop.setcolor(0x00FF00)
canvas.grop.rect(80,80, 40, 10)

app.run()
