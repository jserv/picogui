#!/usr/bin/env python

import PicoGUI
app = PicoGUI.InvisibleApp()
def myfilter(t, sender):
    print (t.dev, t.name, t.sender, t.__dict__)
app.link(myfilter, app.addInfilter())
app.run()
