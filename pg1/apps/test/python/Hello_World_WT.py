#!/usr/bin/env python

# A "simple as it gets" widget template
import PicoGUI
app = PicoGUI.Application('Greetings', PicoGUI.WTFile())
l = app.addWidget('Label')
l.side = 'All'
l.text = 'Hello, World!'
l.font = ':24:Bold'

import sys
if len(sys.argv) > 1:
    fname = sys.argv[1]
else:
    fname = 'hello.pgwt'
open(fname, 'w').write(app.server.dump())
