#!/usr/bin/env python

import PicoGUI, sys
app = PicoGUI.InvisibleApp()
app.server.dup(app.server.mktemplate(open(sys.argv[1]).read()))
app.run()
