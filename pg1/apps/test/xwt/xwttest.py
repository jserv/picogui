#!/usr/bin/env python
#
# Example for XML Widget Templates
#

import PicoGUI
from PicoGUI import XWTParser

parser = XWTParser.XWTParser()
wtBinary = parser.Parse(open("test.xwt").read())

app = PicoGUI.TemplateApp(wtBinary)
app.run()
