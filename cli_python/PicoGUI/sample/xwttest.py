#!/usr/bin/env python
#
# Example for XML Widget Templates
#

import PicoGUI
from PicoGUI import XWTParser

parser = XWTParser.XWTParser()
wtBinary = parser.Parse(open("test.xwt").read())

app = PicoGUI.InvisibleApp()
wtHandle = app.server.mktemplate(wtBinary)
wtInstance = PicoGUI.Widget(app.server,app.server.dup(wtHandle))   
app.run()
