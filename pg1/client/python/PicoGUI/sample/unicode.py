#!/usr/bin/env python

# Demonstrates Unicode support in PicoGUI
import PicoGUI
app = PicoGUI.Application('Unicode Test')
w = app.addWidget('Label')
w.side = 'all'
w.text = u"Hello\u00A9\n\nUnicode\u2260crufty".encode("utf-8")   # Actual unicode text, encoded in UTF-8
w.font = ':0:encoding unicode'                                   # Try to get a font with unicode characters
app.run()
